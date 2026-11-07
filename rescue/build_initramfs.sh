#!/bin/sh
# usage: build_initramfs.sh [authorized_keys] [ice5lp_2k_cal.bin]  -> rescue.cpio.gz
set -e
cd "$(dirname "$0")"
rm -rf stage && cp -a initramfs stage
if [ -n "$1" ]; then mkdir -p stage/root/.ssh; cp "$1" stage/root/.ssh/authorized_keys; fi
if [ -n "$2" ]; then mkdir -p stage/lib/firmware/lattice; cp "$2" stage/lib/firmware/lattice/ice5lp_2k_cal.bin; fi
if [ -f dropbear/dropbear ]; then cp dropbear/dropbear dropbear/dropbearkey stage/sbin/; fi
(cd stage && find . | cpio -o -H newc --owner 0:0 2>/dev/null | gzip -9) > rescue.cpio.gz
ls -l rescue.cpio.gz
