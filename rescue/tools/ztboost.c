// SPDX-License-Identifier: GPL-2.0
/*
 * ztOS stop-gap CPU/CCI clock setup for the Lumia 950 XL (MSM8994), until a real cpufreq driver exists.
 * Bootloader leaves A57 at 381 MHz (PLL L=80, post-div 4) and the CCI at 268.8 MHz (PLL/2).
 * References (downstream drivers/clk/qcom/clock-cpu-8994.c, msm8992-regulator.dtsi):
 *  - A57 mux f900f000+0x54: lfmux bits[2:1] (2 = PLL0 main, 3 = AUX/GPLL0 600 MHz = safe; 1 = PLL1 which is OFF), PLL f9016000
 *    L at +4, mode at +0 (bit0 OUTCTRL, bit1 BYPASSNL, bit2 RESET_N, bit31 lock), USER_CTL +0x10 post-div bits[9:8] (1 = /2, 3 = /4).
 *  - CCI mux f9112000+0x11c: lfmux bits[2:1] (1 = AUX), hfmux bits[4:3] (1 = lfmux, 2 = PLL/2, 3 = PLL direct); PLL f9011000 L=28.
 *  - Rails (PM8994 SPMI usid 1): S11 = A57 (0x3200), S8 = A53 + CCI (0x2900); V(mV) = 80 + 5 * MSB of the 2-byte register at +0x40.
 *    Downstream CPR ceiling for the 768..960 MHz steps is 1000 mV; CPR itself is not in mainline, so the rail is held at the ceiling.
 * 960 MHz is reached on 1-2 loaded A57 cores; the SoC lowers the clock itself with 3 (~900) and 4 (~768) loaded cores.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/spmi.h>

static int l57 = 100;
module_param(l57, int, 0444);
static bool cci = true;
module_param(cci, bool, 0444);

#define S11 0x3200
#define S8  0x2900

static struct spmi_device *pmic(void)
{
	struct device_node *np;
	struct spmi_device *sd = NULL;

	for_each_compatible_node(np, NULL, "qcom,pm8994") {
		u32 usid;
		if (!of_property_read_u32(np, "reg", &usid) && usid == 1) {
			sd = spmi_find_device_by_of_node(np);
			of_node_put(np);
			break;
		}
	}
	return sd;
}

static int rail_get(struct spmi_device *sd, int base)
{
	u8 b[2];
	int r = spmi_ext_register_readl(sd, base + 0x40, b, 2);
	return r ? r : 80 + 5 * b[1];
}

static int rail_set(struct spmi_device *sd, int base, int mv)
{
	u8 b[2] = { 0, (mv - 80) / 5 };
	return spmi_ext_register_writel(sd, base + 0x40, b, 2);
}

static void mw(void __iomem *r, u32 v)
{
	writel(v, r);
	readl(r);
	udelay(50);
}

static int a57_up(void)
{
	void __iomem *mux = ioremap(0xf900f000, 0x1000), *pll = ioremap(0xf9016000, 0x1000);
	u32 m, u;
	int tries = 1000, ret = 0;

	if (!mux || !pll) { ret = -ENOMEM; goto out; }
	m = readl(mux + 0x54);
	u = readl(pll + 0x10);
	if (((m >> 1) & 3) != 2 || ((m >> 3) & 3) != 0 || readl(pll + 8) || readl(pll + 0xc)) {
		pr_warn("ztboost: A57 not in the expected bootloader state (mux %08x), leaving it\n", m);
		goto out;
	}
	mw(mux + 0x54, (m & ~(3 << 1)) | (3 << 1));		/* AUX / GPLL0 */
	writel(readl(pll) & ~1, pll); udelay(10);
	writel(0, pll); udelay(10);
	writel(l57, pll + 4);
	writel((u & ~(0xf << 8)) | (1 << 8), pll + 0x10);
	writel(2, pll); udelay(10);
	writel(6, pll);
	while (tries-- && !(readl(pll) & 0x80000000)) udelay(10);
	if (!(readl(pll) & 0x80000000)) {
		pr_err("ztboost: A57 PLL did not lock; trying the old setting\n");
		writel(0, pll); writel(80, pll + 4); writel((u & ~(0xf << 8)) | (3 << 8), pll + 0x10);
		writel(2, pll); udelay(10); writel(6, pll); tries = 1000;
		while (tries-- && !(readl(pll) & 0x80000000)) udelay(10);
		if (!(readl(pll) & 0x80000000)) { ret = -EIO; goto out; }	/* stays on 600 MHz AUX */
	}
	writel(7, pll); udelay(10);
	mw(mux + 0x54, (readl(mux + 0x54) & ~(3 << 1)) | (2 << 1));
	pr_info("ztboost: A57 PLL L=%u, mux %08x\n", readl(pll + 4), readl(mux + 0x54));
out:
	if (mux) iounmap(mux);
	if (pll) iounmap(pll);
	return ret;
}

static void cci_up(void)
{
	void __iomem *m = ioremap(0xf9112000, 0x1000), *p = ioremap(0xf9011000, 0x1000);
	u32 v;

	if (!m || !p) goto out;
	v = readl(m + 0x11c);
	if (((v >> 3) & 3) != 2 || (readl(p) & 0x80000007) != 0x80000007 || readl(p + 4) != 28) {
		pr_warn("ztboost: CCI not in the expected state (mux %08x), leaving it\n", v);
		goto out;
	}
	mw(m + 0x11c, (v & ~(3 << 1)) | (1 << 1));
	mw(m + 0x11c, (readl(m + 0x11c) & ~(3 << 3)) | (1 << 3));
	mw(m + 0x11c, (readl(m + 0x11c) & ~(3 << 3)) | (3 << 3));
	pr_info("ztboost: CCI mux %08x (PLL direct, 537.6 MHz)\n", readl(m + 0x11c));
out:
	if (m) iounmap(m);
	if (p) iounmap(p);
}

static int __init ztboost_init(void)
{
	struct spmi_device *sd = pmic();
	int a57v, a53v;

	if (!sd) return -ENODEV;
	a57v = rail_get(sd, S11);
	a53v = rail_get(sd, S8);
	pr_info("ztboost: rails S11 %d mV, S8 %d mV\n", a57v, a53v);
	if (a57v < 800 || a57v > 1100 || a53v < 800 || a53v > 1100) {
		pr_err("ztboost: implausible rail readings, doing nothing\n");
		return -EINVAL;
	}
	if (a57v < 1000 && rail_set(sd, S11, 1000)) return -EIO;
	msleep(5);
	if (rail_get(sd, S11) != 1000) { pr_err("ztboost: S11 did not reach 1000 mV\n"); return -EIO; }
	a57_up();
	if (cci && a53v >= 1000) cci_up();
	return 0;
}
static void __exit ztboost_exit(void) {}
module_init(ztboost_init); module_exit(ztboost_exit);
MODULE_DESCRIPTION("ztOS stop-gap A57/CCI clock setup for MSM8994");
MODULE_LICENSE("GPL");
