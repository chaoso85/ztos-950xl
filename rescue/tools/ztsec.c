// SPDX-License-Identifier: GPL-2.0
/* ztOS debug: run the TZ IOMMU secure page-table setup by hand, step by step, from a module. */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/firmware/qcom/qcom_scm.h>

static int pool = 0;		/* 1: IOMMU_SET_CP_POOL_SIZE(300 MiB) first, like msm-3.10 */
module_param(pool, int, 0444);
static int doinit = 1;		/* 0: stop after SIZE */
module_param(doinit, int, 0444);
static char *gdscs = "";	/* "venus,camss,oxili": force these MMSS GDSCs on before INIT */
module_param(gdscs, charp, 0444);

static struct platform_device *pd;

struct g { const char *n; u32 off; };
static const struct g gl[] = {
	{ "venus", 0x1024 }, { "venus0", 0x1040 }, { "venus1", 0x1044 }, { "venus2", 0x1050 },
	{ "mdss", 0x2304 }, { "camss", 0x34a0 }, { "jpeg", 0x35a4 }, { "vfe", 0x36a4 },
	{ "cpp", 0x36d4 }, { "oxilicx", 0x4034 }, { "oxili", 0x4024 }, { "fd", 0x3b64 },
};

static void gdsc_dump_and_set(void)
{
	void __iomem *mm = ioremap(0xfd8c0000, 0x5200);
	int i;

	if (!mm)
		return;
	for (i = 0; i < ARRAY_SIZE(gl); i++) {
		u32 v = readl(mm + gl[i].off);

		pr_info("ztsec: gdsc %-7s @%04x = %08x (%s)\n", gl[i].n, gl[i].off, v,
			(v & BIT(31)) ? "ON" : "off");
		if (strstr(gdscs, gl[i].n) && !(v & BIT(31))) {
			writel(v & ~BIT(0), mm + gl[i].off);	/* SW_COLLAPSE = 0 -> power on */
			msleep(2);
			pr_info("ztsec: gdsc %s forced on -> %08x\n", gl[i].n, readl(mm + gl[i].off));
		}
	}
	iounmap(mm);
}

static int __init ztsec_init(void)
{
	size_t psize = 0;
	dma_addr_t pa;
	void *va;
	int ret;

	pr_info("ztsec: start pool=%d doinit=%d gdscs='%s'\n", pool, doinit, gdscs);
	gdsc_dump_and_set();
	if (pool) {
		ret = qcom_scm_iommu_set_cp_pool_size(0, 300 * SZ_1M);
		pr_info("ztsec: SET_CP_POOL_SIZE ret %d\n", ret);
		msleep(200);
	}
	ret = qcom_scm_iommu_secure_ptbl_size(0, &psize);
	pr_info("ztsec: PTBL_SIZE ret %d size %zu\n", ret, psize);
	msleep(200);
	if (ret || !doinit)
		return -EAGAIN;

	pd = platform_device_register_simple("ztsec", -1, NULL, 0);
	if (IS_ERR(pd))
		return PTR_ERR(pd);
	dma_coerce_mask_and_coherent(&pd->dev, DMA_BIT_MASK(32));
	va = dma_alloc_attrs(&pd->dev, psize, &pa, GFP_KERNEL, DMA_ATTR_NO_KERNEL_MAPPING);
	pr_info("ztsec: pool at %pad (%s)\n", &pa, va ? "ok" : "FAIL");
	if (!va)
		goto out;
	msleep(500);
	pr_info("ztsec: calling PTBL_INIT now\n");
	msleep(300);
	ret = qcom_scm_iommu_secure_ptbl_init(pa, psize, 0);
	pr_info("ztsec: PTBL_INIT ret %d\n", ret);
	/* on success the pool now belongs to TZ: never free it */
out:
	return -EAGAIN;	/* never stay loaded; can re-run SIZE-only variants */
}
module_init(ztsec_init);
MODULE_LICENSE("GPL");
