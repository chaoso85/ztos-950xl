// SPDX-License-Identifier: GPL-2.0
/* ztOS: read the three TSENS calibration words (downstream msm8974-tsens.c TSENS_8994_EEPROM = base + 0xd0, base = 0xfc4bc000
 * corrected alias, same alias the stock DSDT uses for the speed bin at +0xb0). 32-bit reads, nothing else is touched. */
#include <linux/module.h>
#include <linux/io.h>
static int __init z_init(void)
{
	void __iomem *p = ioremap(0xfc4bc0d0, 12);
	u32 a, b, c;
	if (!p) return -ENOMEM;
	a = readl(p); b = readl(p + 4); c = readl(p + 8);
	pr_info("ztfuse2: calib %08x %08x %08x, cal_sel %u, base0 %u base1 %u\n", a, b, c, (c >> 20) & 7, a & 0x3ff, (a >> 10) & 0x3ff);
	iounmap(p);
	return 0;
}
static void __exit z_exit(void) {}
module_init(z_init); module_exit(z_exit);
MODULE_LICENSE("GPL");
