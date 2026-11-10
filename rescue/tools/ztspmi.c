// SPDX-License-Identifier: GPL-2.0
/* ztOS debug (read only): dump the PM8994 buck registers that set the CPU cluster rails (S8 = A53, S11 = A57) and their neighbours. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spmi.h>

static const struct { const char *n; u16 base; } rails[] = {
	{ "s1 ", 0x1400 }, { "s2 ", 0x1700 }, { "s3 ", 0x1a00 }, { "s5 ", 0x2000 }, { "s6 ", 0x2300 }, { "s7 ", 0x2600 },
	{ "s8 ", 0x2900 }, { "s9 ", 0x2c00 }, { "s10", 0x2f00 }, { "s11", 0x3200 }, { "s12", 0x3500 },
};

static int __init ztspmi_init(void)
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
	if (!sd) {
		pr_err("ztspmi: pm8994 usid 1 not found\n");
		return -ENODEV;
	}
	for (int i = 0; i < ARRAY_SIZE(rails); i++) {
		u8 b[0x50] = {};
		u8 v[4];
		int r = 0, ok = 1;
		static const u16 offs[] = { 0x04, 0x05, 0x08, 0x40, 0x41, 0x45, 0x46 };

		for (int k = 0; k < ARRAY_SIZE(offs); k++) {
			r = spmi_ext_register_readl(sd, rails[i].base + offs[k], &b[offs[k]], 1);
			if (r) { ok = 0; break; }
		}
		(void)v;
		if (!ok)
			pr_info("ztspmi: %s base %04x read error %d\n", rails[i].n, rails[i].base, r);
		else
			pr_info("ztspmi: %s type %02x sub %02x status1 %02x vctl_lsb %02x vctl_msb %02x mode %02x enable %02x\n",
				rails[i].n, b[4], b[5], b[8], b[0x40], b[0x41], b[0x45], b[0x46]);
	}
	return 0;
}
static void __exit ztspmi_exit(void) { }
module_init(ztspmi_init);
module_exit(ztspmi_exit);
MODULE_LICENSE("GPL");
