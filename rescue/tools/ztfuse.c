// SPDX-License-Identifier: GPL-2.0
/*
 * ztOS: read the CPU speed bin the way the Lumia's own firmware does. Reference: the stock ACPI DSDT (plat/ACPI/dsdt.dsl):
 *   OperationRegion (FUSE, SystemMemory, 0xFC4B8000, 0x5000); Field (FUSE, QWordAcc) { Offset (0x40B0), PTE, 64 }
 *   SSBV = PTE & 7;  0 -> "2.0GHz", 1 -> "1.8GHz".
 * i.e. ONE 64-bit read at 0xFC4BC0B0 (the corrected alias). Reading 0xfc4b80b0 (raw alias, what the downstream Android driver reads)
 * and 0xfc4bc000 reset this phone, so nothing else may be touched here.
 */
#include <linux/module.h>
#include <linux/io.h>

static int __init ztfuse_init(void)
{
	void __iomem *p = ioremap(0xfc4bc0b0, 8);
	u64 pte;

	if (!p) return -ENOMEM;
	pte = readq(p);
	pr_info("ztfuse: PTE %016llx, speed bin %llu (%s)\n", pte, pte & 7,
		(pte & 7) == 0 ? "2.0GHz" : (pte & 7) == 1 ? "1.8GHz" : "unknown");
	iounmap(p);
	return 0;
}
static void __exit ztfuse_exit(void) {}
module_init(ztfuse_init); module_exit(ztfuse_exit);
MODULE_LICENSE("GPL");
