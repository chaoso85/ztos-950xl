#!/bin/sh
# ztOS debug: load msm DRM + panel (built as modules); order follows each module's depends=
cd /mnt/sd/ztos/ko || exit 1
for m in cec drm_display_helper gpu-sched drm_exec drm_gpuvm drm_dp_aux_bus ubwc_config panel-microsoft-sergej msm; do
  echo "[loaddrm] insmod $m" > /dev/kmsg
  insmod $m.ko || echo "[loaddrm] insmod $m failed" > /dev/kmsg
done
