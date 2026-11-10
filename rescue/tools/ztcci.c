// SPDX-License-Identifier: GPL-2.0
/*
 * ztOS experiment: CCI (cache coherent interconnect) clock on MSM8994/8992.
 * Reference: downstream clock-cpu-8994.c: cci mux at f9112000 + 0x11c: lfmux sel bits[2:1] (1 = AUX/GPLL0 600 MHz, 0 = XO),
 * hfmux sel bits[4:3] (1 = lfmux, 2 = cci_pll_main = PLL/2, 3 = PLL direct), divider bits[6:5]; CCI PLL f9011000, L = reg +4.
 * Bootloader state: PLL L=28 (537.6 MHz), hfmux 2 -> CCI runs at 268.8 MHz. op=up -> hfmux 3 (537.6 MHz), op=down back.
 * Both go through the AUX source (hfmux 1, lfmux 1) like the downstream safe_parent logic. Rail S8 must be >= 1.0 V.
 */
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>

static char *op = "status";
module_param(op, charp, 0444);
#define MUXO 0x11c

static u32 rd(void __iomem *m) { return readl(m + MUXO); }

static void wr(void __iomem *m, u32 v)
{
	writel(v, m + MUXO);
	readl(m + MUXO);
	udelay(50);
}

static int __init ztcci_init(void)
{
	void __iomem *m = ioremap(0xf9112000, 0x1000), *p = ioremap(0xf9011000, 0x1000);
	u32 v;

	if (!m || !p) return -ENODEV;
	v = rd(m);
	pr_info("ztcci: mux %08x (lf %u hf %u div %u), pll mode %08x L %u user_ctl %08x\n", v, (v >> 1) & 3, (v >> 3) & 3, (v >> 5) & 3,
		readl(p), readl(p + 4), readl(p + 0x10));
	if (!strcmp(op, "up") || !strcmp(op, "down")) {
		u32 hf = !strcmp(op, "up") ? 3 : 2;
		wr(m, (v & ~(3 << 1)) | (1 << 1));		/* lfmux -> AUX */
		pr_info("ztcci: lf on AUX, mux %08x\n", rd(m)); msleep(100);
		wr(m, (rd(m) & ~(3 << 3)) | (1 << 3));	/* hfmux -> lfmux (safe) */
		pr_info("ztcci: hf on safe, mux %08x\n", rd(m)); msleep(100);
		wr(m, (rd(m) & ~(3 << 3)) | (hf << 3));	/* hfmux -> PLL output */
		pr_info("ztcci: hf %u, mux %08x\n", hf, rd(m));
	}
	iounmap(m); iounmap(p);
	return 0;
}
static void __exit ztcci_exit(void) {}
module_init(ztcci_init); module_exit(ztcci_exit);
MODULE_LICENSE("GPL");
