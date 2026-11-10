// SPDX-License-Identifier: GPL-2.0
/*
 * ztOS experiment: raise the Cortex-A57 cluster (MSM8994) from the bootloader's 384 MHz to 768 MHz, the way the downstream
 * clock-cpu-8994.c does it, without a real driver: first the rail (PM8994 S11, 0.9 V -> 1.0 V, the NOM voltage ceiling of the
 * downstream apc1 regulator), then mux to the safe source, PLL post-divider, mux back. op=down reverses it, op=status only reads.
 * A57 mux f900f000+0x54: lfmux sel in bits[2:1] (2 = PLL0 main, 3 = AUX/GPLL0 600 MHz = safe (sel 1 is PLL1/PLLA, which is OFF -> hang)), PLL f9016000 USER_CTL +0x10: post-div field bits[11:8].
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/spmi.h>
#include <linux/firmware/qcom/qcom_scm.h>

static char *op = "status";
module_param(op, charp, 0444);

#define MUX_PHYS	0xf900f000
#define PLL_PHYS	0xf9016000
#define MUX_OFF		0x54
#define PLL_USER_CTL	0x10
#define S11_BASE	0x3200

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

static int s11_get(struct spmi_device *sd)
{
	u8 b[2];
	int r = spmi_ext_register_readl(sd, S11_BASE + 0x40, b, 2);
	return r ? r : 80 + 5 * b[1];	/* mV, from the readings of s8/s9/s11/s1: V = 80 + 5 * MSB */
}

static int s11_set(struct spmi_device *sd, int mv)
{
	u8 b[2] = { 0, (mv - 80) / 5 };
	return spmi_ext_register_writel(sd, S11_BASE + 0x40, b, 2);
}

static int __init ztcpu_init(void)
{
	void __iomem *mux = ioremap(MUX_PHYS, 0x1000), *pll = ioremap(PLL_PHYS, 0x1000);
	struct spmi_device *sd = pmic();
	u32 m, u;
	int v;

	if (!mux || !pll || !sd)
		return -ENODEV;
	m = readl(mux + MUX_OFF);
	u = readl(pll + PLL_USER_CTL);
	v = s11_get(sd);
	pr_info("ztcpu: before: S11 %d mV, A57 mux %08x (lf sel %u), pll user_ctl %08x (post-div field %u)\n", v, m, (m >> 1) & 3, u, (u >> 8) & 0xf);

	if (!strcmp(op, "sread")) {			/* read the mux and the PLL user_ctl through TZ (SCM io read) */
		u32 sm = 0, su = 0;
		int r1 = qcom_scm_io_readl(MUX_PHYS + MUX_OFF, &sm);
		int r2 = qcom_scm_io_readl(PLL_PHYS + PLL_USER_CTL, &su);
		pr_info("ztcpu: scm read mux %08x (%d), pll user_ctl %08x (%d)\n", sm, r1, su, r2);
	} else if (!strcmp(op, "smux")) {		/* mux to the safe source and back, written through TZ */
		u32 sm = 0;
		int r = qcom_scm_io_readl(MUX_PHYS + MUX_OFF, &sm);
		pr_info("ztcpu: scm mux read %08x (%d)\n", sm, r); msleep(300);
		r = qcom_scm_io_writel(MUX_PHYS + MUX_OFF, (sm & ~(3 << 1)) | (1 << 1));
		udelay(50);
		pr_info("ztcpu: scm write safe: %d, mux now %08x\n", r, readl(mux + MUX_OFF)); msleep(300);
		r = qcom_scm_io_writel(MUX_PHYS + MUX_OFF, sm);
		udelay(50);
		pr_info("ztcpu: scm write back: %d, mux now %08x\n", r, readl(mux + MUX_OFF));
	} else if (!strcmp(op, "sdiv")) {		/* needs the rail at 1000 mV; mux + PLL post-div written through TZ */
		u32 sm = 0, su = 0;
		if (s11_get(sd) != 1000) { pr_err("ztcpu: rail is not at 1000 mV, refusing\n"); goto out; }
		qcom_scm_io_readl(MUX_PHYS + MUX_OFF, &sm);
		qcom_scm_io_readl(PLL_PHYS + PLL_USER_CTL, &su);
		pr_info("ztcpu: sdiv start mux %08x user_ctl %08x\n", sm, su); msleep(300);
		qcom_scm_io_writel(MUX_PHYS + MUX_OFF, (sm & ~(3 << 1)) | (1 << 1));
		udelay(50);
		pr_info("ztcpu: on safe, writing post-div\n"); msleep(300);
		qcom_scm_io_writel(PLL_PHYS + PLL_USER_CTL, (su & ~(0xf << 8)) | (1 << 8));
		udelay(100);
		qcom_scm_io_readl(PLL_PHYS + PLL_USER_CTL, &su);
		pr_info("ztcpu: user_ctl now %08x, back to PLL0\n", su); msleep(300);
		qcom_scm_io_writel(MUX_PHYS + MUX_OFF, sm);
		udelay(50);
		pr_info("ztcpu: done, mux %08x\n", readl(mux + MUX_OFF));
	} else if (!strcmp(op, "sdivdown")) {
		u32 sm = 0, su = 0;
		qcom_scm_io_readl(MUX_PHYS + MUX_OFF, &sm);
		qcom_scm_io_readl(PLL_PHYS + PLL_USER_CTL, &su);
		qcom_scm_io_writel(MUX_PHYS + MUX_OFF, (sm & ~(3 << 1)) | (1 << 1));
		udelay(50);
		qcom_scm_io_writel(PLL_PHYS + PLL_USER_CTL, (su & ~(0xf << 8)) | (3 << 8));
		udelay(100);
		qcom_scm_io_writel(MUX_PHYS + MUX_OFF, sm);
		udelay(50);
		pr_info("ztcpu: s11 -> 900 mV: %d\n", s11_set(sd, 900));
	} else if (!strcmp(op, "volt")) {			/* step 1 only: the rail */
		pr_info("ztcpu: S11 -> 1000 mV: %d\n", s11_set(sd, 1000));
		msleep(20);
		pr_info("ztcpu: S11 now %d mV\n", s11_get(sd));
	} else if (!strcmp(op, "voltdown")) {
		pr_info("ztcpu: S11 -> 900 mV: %d\n", s11_set(sd, 900));
		msleep(20);
		pr_info("ztcpu: S11 now %d mV\n", s11_get(sd));
	} else if (!strcmp(op, "mux")) {		/* step 2 only: A57 to the safe source and back, PLL untouched */
		pr_info("ztcpu: mux -> safe\n"); msleep(200);
		writel((m & ~(3 << 1)) | (3 << 1), mux + MUX_OFF);
		readl(mux + MUX_OFF);
		udelay(50);
		pr_info("ztcpu: on safe source, mux %08x\n", readl(mux + MUX_OFF)); msleep(200);
		writel((readl(mux + MUX_OFF) & ~(3 << 1)) | (2 << 1), mux + MUX_OFF);
		readl(mux + MUX_OFF);
		udelay(50);
		pr_info("ztcpu: back on PLL0, mux %08x\n", readl(mux + MUX_OFF));
	} else if (!strcmp(op, "div")) {		/* step 3: needs volt first */
		if (s11_get(sd) != 1000) { pr_err("ztcpu: rail is not at 1000 mV, refusing\n"); goto out; }
		pr_info("ztcpu: mux -> safe\n"); msleep(200);
		writel((m & ~(3 << 1)) | (3 << 1), mux + MUX_OFF);
		readl(mux + MUX_OFF);
		udelay(50);
		pr_info("ztcpu: PLL post-div 3 -> 1\n"); msleep(200);
		writel((u & ~(0xf << 8)) | (1 << 8), pll + PLL_USER_CTL);
		readl(pll + PLL_USER_CTL);
		udelay(100);
		pr_info("ztcpu: user_ctl %08x, mux back to PLL0\n", readl(pll + PLL_USER_CTL)); msleep(200);
		writel((readl(mux + MUX_OFF) & ~(3 << 1)) | (2 << 1), mux + MUX_OFF);
		readl(mux + MUX_OFF);
		udelay(50);
		pr_info("ztcpu: done, mux %08x\n", readl(mux + MUX_OFF));
	} else if (!strcmp(op, "divdown")) {
		writel((m & ~(3 << 1)) | (3 << 1), mux + MUX_OFF);
		readl(mux + MUX_OFF);
		udelay(50);
		writel((u & ~(0xf << 8)) | (3 << 8), pll + PLL_USER_CTL);
		readl(pll + PLL_USER_CTL);
		udelay(100);
		writel((readl(mux + MUX_OFF) & ~(3 << 1)) | (2 << 1), mux + MUX_OFF);
		readl(mux + MUX_OFF);
		udelay(50);
	}
out:
	pr_info("ztcpu: after: S11 %d mV, A57 mux %08x, pll user_ctl %08x\n", s11_get(sd), readl(mux + MUX_OFF), readl(pll + PLL_USER_CTL));
	iounmap(mux);
	iounmap(pll);
	return 0;
}
static void __exit ztcpu_exit(void) { }
module_init(ztcpu_init);
module_exit(ztcpu_exit);
MODULE_LICENSE("GPL");
