// SPDX-License-Identifier: GPL-2.0
/* ztOS debug: set status = "okay" on a DT node at runtime; the OF reconfig notifiers create its device. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>

static char *node = "";
module_param(node, charp, 0444);

static int __init ztdt_init(void)
{
	struct of_changeset *cs;
	struct device_node *np;
	struct property *p;
	int ret;

	np = of_find_node_by_path(node);
	if (!np) {
		pr_err("ztdt: %s not found\n", node);
		return -ENODEV;
	}
	pr_info("ztdt: enabling %pOF (available=%d)\n", np, of_device_is_available(np));
	cs = kzalloc(sizeof(*cs), GFP_KERNEL);
	p = kzalloc(sizeof(*p), GFP_KERNEL);
	if (!cs || !p)
		return -ENOMEM;
	p->name = kstrdup("status", GFP_KERNEL);
	p->value = kstrdup("okay", GFP_KERNEL);
	p->length = 5;
	of_changeset_init(cs);
	ret = of_changeset_update_property(cs, np, p);
	if (!ret)
		ret = of_changeset_apply(cs);
	pr_info("ztdt: %pOF status okay -> %d (available=%d)\n", np, ret, of_device_is_available(np));
	of_node_put(np);
	return 0;
}
static void __exit ztdt_exit(void) { }
module_init(ztdt_init);
module_exit(ztdt_exit);
MODULE_LICENSE("GPL");
