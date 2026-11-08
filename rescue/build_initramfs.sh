#!/bin/sh
# usage: build_initramfs.sh [authorized_keys] [ice5lp_2k_cal.bin] [firmware_dir]  -> rescue.cpio.gz
# firmware_dir (e.g. fw/) is copied to /lib/firmware, e.g. fw/qcom/a420_pm4.fw from linux-firmware (not in git)
set -e
cd "$(dirname "$0")"
rm -rf stage && cp -a initramfs stage
if [ -n "$1" ]; then mkdir -p stage/root/.ssh; cp "$1" stage/root/.ssh/authorized_keys; fi
if [ -n "$2" ]; then mkdir -p stage/lib/firmware/lattice; cp "$2" stage/lib/firmware/lattice/ice5lp_2k_cal.bin; fi
if [ -n "$3" ] && [ -d "$3" ]; then mkdir -p stage/lib/firmware; cp -a "$3"/. stage/lib/firmware/; fi
cp mmio ulpi stage/sbin/ 2>/dev/null
if [ -f dropbear/dropbear ]; then cp dropbear/dropbear dropbear/dropbearkey stage/sbin/; fi
(cd stage && find . | cpio -o -H newc --owner 0:0 2>/dev/null | gzip -9) > rescue.cpio.gz
ls -l rescue.cpio.gz
