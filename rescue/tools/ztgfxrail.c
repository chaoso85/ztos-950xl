// SPDX-License-Identifier: GPL-2.0
/* ztOS debug (read only): dump the pmi8994 S2/S3 bucks (VDD_GFX rail, SPMI usid 3) to see the real voltage/enable state.
 * Same register layout as ztspmi.c: base+0x04 type, +0x05 subtype, +0x08 status1, +0x40/0x41 vctl, +0x45 mode, +0x46 enable. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spmi.h>
static const struct { const char *n; u16 base; } rails[] = { { "s1", 0x1400 }, { "s2", 0x1700 }, { "s3", 0x1a00 } };
static int __init z_init(void)
{
	struct device_node *np;
	struct spmi_device *sd = NULL;
	for_each_compatible_node(np, NULL, "qcom,pmi8994") {
		u32 usid;
		if (!of_property_read_u32(np, "reg", &usid) && usid == 3) {
			sd = spmi_find_device_by_of_node(np);
			of_node_put(np);
			break;
		}
	}
	if (!sd) { pr_err("ztgfxrail: pmi8994 usid 3 not found\n"); return -ENODEV; }
	for (int i = 0; i < ARRAY_SIZE(rails); i++) {
		static const u16 offs[] = { 0x04, 0x05, 0x08, 0x40, 0x41, 0x45, 0x46 };
		u8 b[0x50] = {};
		int r = 0, k;
		for (k = 0; k < ARRAY_SIZE(offs); k++) {
			r = spmi_ext_register_readl(sd, rails[i].base + offs[k], &b[offs[k]], 1);
			if (r) break;
		}
		if (r) pr_info("ztgfxrail: %s base %04x read error %d\n", rails[i].n, rails[i].base, r);
		else pr_info("ztgfxrail: %s type %02x sub %02x status1 %02x vctl_lsb %02x vctl_msb %02x mode %02x enable %02x\n",
			     rails[i].n, b[4], b[5], b[8], b[0x40], b[0x41], b[0x45], b[0x46]);
	}
	return 0;
}
static void __exit z_exit(void) {}
module_init(z_init); module_exit(z_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ztOS read-only VDD_GFX rail dump");
