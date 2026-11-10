// SPDX-License-Identifier: GPL-2.0
/* ztOS: bring the MSM8994 TSENS up by hand and print what the 16 sensors read.
 * Reference: downstream msm8974-tsens.c tsens_hw_init() (local-init branch): CTRL = 0, SW_RST, then
 * CTRL = EN | sensors<<3 | 62.5 ms period<<18; GLOBAL_CONFIG(+0x34) = 0x302f16c; S0_MAIN_CONFIG(+0x38) = 0x1c3;
 * SN_REMOTE_CONFIG(+0x3c + 4n) = 0x11c3. SROT = 0xfc4a8000, TM (status +0x30 + 4n, TRDY +0x5c) = 0xfc4a9000.
 * Calibration (ztfuse2): base0 526 (30 degC), base1 833 (120 degC) -> slope 3.411 codes/degC, offsets all 0. */
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
static int do_init = 1;
module_param(do_init, int, 0444);
static int __init z_init(void)
{
	void __iomem *srot = ioremap(0xfc4a8000, 0x1000), *tm = ioremap(0xfc4a9000, 0x1000);
	int i;
	if (!srot || !tm) return -ENOMEM;
	pr_info("ztts: before: SROT CTRL %08x GLOBAL %08x S0 %08x, TM INT_EN %08x TRDY %08x\n", readl(srot), readl(srot + 0x34), readl(srot + 0x38), readl(tm), readl(tm + 0x5c));
	if (do_init) {
		writel(0, srot);
		writel(2, srot);
		writel(0, srot);
		writel((1 << 18) | (0xffff << 3) | 1, srot);
		writel(0x302f16c, srot + 0x34);
		writel(0x1c3, srot + 0x38);
		for (i = 0; i < 16; i++) writel(0x11c3, srot + 0x3c + 4 * i);
		writel(0, tm);		/* no interrupts for this test */
		msleep(500);
		pr_info("ztts: after: SROT CTRL %08x GLOBAL %08x, TRDY %08x\n", readl(srot), readl(srot + 0x34), readl(tm + 0x5c));
	}
	for (i = 0; i < 16; i++) {
		u32 v = readl(tm + 0x30 + 4 * i), code = v & 0x3ff;
		int mc = 30000 + (int)(((int)code - 526) * 90000 / 307);
		pr_info("ztts: sensor %2d status %08x code %4u -> %d.%d C\n", i, v, code, mc / 1000, abs(mc % 1000) / 100);
	}
	iounmap(srot); iounmap(tm);
	return 0;
}
static void __exit z_exit(void) {}
module_init(z_init); module_exit(z_exit);
MODULE_LICENSE("GPL");
