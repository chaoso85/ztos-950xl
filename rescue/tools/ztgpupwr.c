// SPDX-License-Identifier: GPL-2.0
/* ztOS: power the Adreno 430 GX + CX GDSCs by hand (Windows PEP / downstream kgsl order) and read their
 * status. Touches neither the GPU SMMU nor the GPU registers. GDSCR: GX mmcc+0x4024, CX mmcc+0x4034
 * (bit31 PWR_ON, bit0 SW_COLLAPSE). Reference: docs/research/GPU_DRIVER_ANALYSIS.md. */
#include <linux/module.h>
#include <linux/io.h>
#include <soc/qcom/msm8994-oxili.h>
static void dump(const char *tag, void __iomem *m)
{
	pr_info("ztgpupwr: %s GX GDSCR %08x (CFG %08x)  CX GDSCR %08x\n", tag,
		readl(m + 0x4024), readl(m + 0x4028), readl(m + 0x4034));
}
static int __init z_init(void)
{
	void __iomem *m = ioremap(0xfd8c0000, 0x5200);
	int ret;
	if (!m) return -ENOMEM;
	dump("before", m);
	ret = msm8994_oxili_pre_gpu_power();
	pr_info("ztgpupwr: pre_gpu_power ret=%d\n", ret);
	dump("after", m);
	iounmap(m);
	return 0;
}
static void __exit z_exit(void) {}
module_init(z_init); module_exit(z_exit);
MODULE_LICENSE("GPL");
