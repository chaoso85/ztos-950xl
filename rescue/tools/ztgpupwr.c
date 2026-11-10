// SPDX-License-Identifier: GPL-2.0
/* ztOS: power the Adreno 430 GX + CX GDSCs by hand (Windows PEP / downstream kgsl order), enable the GPU
 * clocks, and (gpu=1 / smmu=1) read the first GPU registers / the GPU SMMU ID registers.
 * smmu=1 hangs the CPU (tested 2026-11-10); gpu=1 is the next test. Never writes the SMMU or the GPU.
 * GDSCR: GX mmcc+0x4024, CX mmcc+0x4034 (bit31 PWR_ON, bit0 SW_COLLAPSE).
 * Reference: docs/research/GPU_DRIVER_ANALYSIS.md. */
#include <linux/module.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/clk-provider.h>
#include <dt-bindings/clock/qcom,mmcc-msm8994.h>
#include <dt-bindings/clock/qcom,rpmcc.h>
#include <soc/qcom/msm8994-oxili.h>
static int smmu, gpu, rail = 1, cfgbus = 1, noc = 1;
module_param(noc, int, 0444);
module_param(cfgbus, int, 0444);
/* RPM bus-arbiter request (exported by drivers/interconnect/qcom/smd-rpm.c) */
int qcom_icc_rpm_smd_send(int ctx, int rsc_type, int id, u32 val);
#define ZT_BUS_MASTER_REQ 0x73616d62
#define ZT_BUS_SLAVE_REQ  0x766c7362
module_param(rail, int, 0444);
static struct platform_device *rpdev;
module_param(smmu, int, 0444);
module_param(gpu, int, 0444);
static void dump(const char *tag, void __iomem *m)
{
	pr_info("ztgpupwr: %s GX GDSCR %08x (CFG %08x)  CX GDSCR %08x\n", tag,
		readl(m + 0x4024), readl(m + 0x4028), readl(m + 0x4034));
}
static struct clk *getclk(struct device_node *np, int id)
{
	struct of_phandle_args a = { .np = np, .args_count = 1, .args = { id } };
	return of_clk_get_from_provider(&a);
}
static int __init z_init(void)
{
	void __iomem *m = ioremap(0xfd8c0000, 0x5200);
	struct device_node *np = of_find_compatible_node(NULL, NULL, "qcom,mmcc-msm8994");
	struct clk *cx_ahb, *gfx3d, *rbbm;
	int ret;
	if (!m || !np) return -ENODEV;
	dump("before", m);
	if (rail) {
		/* vote the RPM VDD_GFX corner (gpu node power-domains, required-opps = svs_soc = 3) first */
		struct device_node *g = of_find_compatible_node(NULL, NULL, "qcom,adreno-430.0");
		struct of_phandle_args pa;
		if (g && !of_parse_phandle_with_args(g, "power-domains", "#power-domain-cells", 0, &pa)) {
			rpdev = platform_device_register_simple("ztgpu-rail", -1, NULL, 0);
			if (!IS_ERR(rpdev)) {
				ret = of_genpd_add_device(&pa, &rpdev->dev);
				pm_runtime_enable(&rpdev->dev);
				pr_info("ztgpupwr: genpd add=%d\n", ret);
				pr_info("ztgpupwr: perf state(3)=%d\n", dev_pm_genpd_set_performance_state(&rpdev->dev, 3));
				pr_info("ztgpupwr: rail resume=%d\n", pm_runtime_resume_and_get(&rpdev->dev));
			}
			of_node_put(pa.np);
		} else
			pr_info("ztgpupwr: no gpu power-domains node\n");
	}
	ret = msm8994_oxili_pre_gpu_power();
	pr_info("ztgpupwr: pre_gpu_power ret=%d\n", ret);
	dump("after power", m);
	if (cfgbus) {
		/* Windows PEP: BUSARB ICBID_MASTER_APPSS_PROC (0) -> ICBID_SLAVE_GFX3D_CFG (11), 0x11E1A300 = 300000000,
		 * after the footswitches and before the clocks */
		pr_info("ztgpupwr: cfgbus master0=%d slave11=%d\n",
			qcom_icc_rpm_smd_send(0, ZT_BUS_MASTER_REQ, 0, 300000000),
			qcom_icc_rpm_smd_send(0, ZT_BUS_SLAVE_REQ, 11, 300000000));
	}
	if (noc) {
		/* MMSS NoC AHB (RPM clock) was never voted by Linux; the Windows GFX3D_CFG bus vote is its hardware counterpart */
		struct device_node *rn = of_find_compatible_node(NULL, NULL, "qcom,rpmcc-msm8994");
		struct clk *nc = rn ? getclk(rn, RPM_SMD_MMSSNOC_AHB_CLK) : ERR_PTR(-ENODEV);
		if (!IS_ERR(nc)) {
			pr_info("ztgpupwr: mmssnoc_ahb set_rate=%d\n", clk_set_rate(nc, 80000000));
			pr_info("ztgpupwr: mmssnoc_ahb enable=%d rate=%lu\n", clk_prepare_enable(nc), clk_get_rate(nc));
		} else
			pr_info("ztgpupwr: mmssnoc_ahb clk err %ld\n", PTR_ERR(nc));
	}
	cx_ahb = getclk(np, OXILICX_AHB_CLK);
	gfx3d = getclk(np, OXILI_GFX3D_CLK);
	rbbm = getclk(np, OXILI_RBBMTIMER_CLK);
	pr_info("ztgpupwr: clks %ld %ld %ld\n", PTR_ERR_OR_ZERO(cx_ahb), PTR_ERR_OR_ZERO(gfx3d), PTR_ERR_OR_ZERO(rbbm));
	if (!IS_ERR(gfx3d)) pr_info("ztgpupwr: gfx3d enable=%d rate=%lu\n", clk_prepare_enable(gfx3d), clk_get_rate(gfx3d));
	if (!IS_ERR(cx_ahb)) pr_info("ztgpupwr: cx_ahb enable=%d\n", clk_prepare_enable(cx_ahb));
	if (!IS_ERR(rbbm)) pr_info("ztgpupwr: rbbmtimer enable=%d\n", clk_prepare_enable(rbbm));
	dump("after clocks", m);
	if (gpu) {
		void __iomem *g = ioremap(0xfdb00000, 0x100);
		if (g) {
			pr_info("ztgpupwr: GPU reg0 (RBBM_HW_VERSION) %08x reg1 %08x reg2 %08x reg3 %08x\n",
				readl(g), readl(g + 4), readl(g + 8), readl(g + 12));
			iounmap(g);
		}
	}
	if (smmu) {
		void __iomem *s = ioremap(0xfdb10000, 0x100);
		if (s) {
			pr_info("ztgpupwr: SMMU ID0 %08x ID1 %08x ID2 %08x\n",
				readl(s + 0x20), readl(s + 0x24), readl(s + 0x28));
			iounmap(s);
		}
	}
	iounmap(m);
	return 0;
}
static void __exit z_exit(void) {}
module_init(z_init); module_exit(z_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ztOS Adreno 430 GX/CX power test");
