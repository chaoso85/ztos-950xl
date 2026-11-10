<div align="center">

# ztOS-950XL

**Bringing [ztOS](https://github.com/Zerone-Laboratories/ztOS) (Linux + the ztMETRO shell) to the Microsoft Lumia 950 XL**

![status](https://img.shields.io/badge/status-early%20bring--up-orange)
![device](https://img.shields.io/badge/device-Lumia%20950%20XL%20(cityman)-blue)
![soc](https://img.shields.io/badge/SoC-Snapdragon%20810%20(MSM8994)-red)
![kernel](https://img.shields.io/badge/kernel-mainline%20based-success)
![boot](https://img.shields.io/badge/boot-UEFI%20%2B%20SD%20card-lightgrey)

[English](README.md) · [Türkçe](README.tr.md)

</div>

> **Status: early bring-up.** The kernel boots on the phone from an SD card and the **ztOS root filesystem (ztMETRO shell) runs from the card's second partition**, rendered by the **Adreno 430 GPU** (Mesa freedreno, 60 fps UI). The panel works with `drm/msm` (1440x2560), the USB rescue shell (SSH) and the SD card work, the Cortex-A57 cluster and the CCI are raised at boot (960 / 538 MHz, bootloader default 384 / 269 MHz) and the SoC temperature sensors are exposed as thermal zones. Camera, audio and Wi-Fi do not work yet. Display reset root cause: [`docs/research/MDP_SMMU_RESET.md`](docs/research/MDP_SMMU_RESET.md). Full chronology: [`docs/research/en/BRINGUP_LOG.md`](docs/research/en/BRINGUP_LOG.md) (also in [Turkish](docs/research/BRINGUP_LOG.md)). Open items: [`docs/OPEN_ITEMS.md`](docs/OPEN_ITEMS.md). Do not flash anything from this repo to your only device without a full eMMC backup.

## Goal

Run a real Linux phone OS on the Lumia 950 XL (RM-1085), without touching Windows: boot from an **SD card**, leave the eMMC (Windows 10 Mobile / Windows on ARM) alone. If it gets stable, ship it as a generic flashable image, like LumiaWOA does.

## Why this is feasible

| Piece | Where it comes from |
|---|---|
| Userland, shell, apps | ztOS (GSI + `ztOSIAL` device layer) |
| Bootloader | Bootloader-unlocked phone + Windows-on-ARM UEFI (Lumia950XLPkg) |
| Kernel base | Mainline Linux + the [Android4Lumia950/linux](https://github.com/Android4Lumia950/linux) work (display, GPU, camera, USB-C, charger, audio) |
| Hardware facts | Offline analysis of the official Windows 10 Mobile FFU (ACPI, drivers, firmware, panel and camera configs) |
| GPU | `drm/msm` (Adreno 4xx) + Mesa `freedreno` — no Windows driver is reused |

## Architecture

```
SD card  ──►  Windows-on-ARM UEFI (eMMC, untouched) ──► rEFInd
                 │  GRUB on the SD (\EFI\BOOT\BOOTAA64.EFI, /ztos/grub.cfg)
                 ▼
           Linux kernel (mainline + Lumia patches) + cityman DTB   (acpi=no, device tree)
                 │
                 ▼
           rescue initramfs (USB gadget + SSH, SD logger, CPU/CCI boost)
                 │  ztos.root=LABEL=ZR_ARCH → switch_root
                 ▼
           ztOS root on SD partition 2 (Arch Linux ARM, ztMETRO, ztOSIAL)
```

## Hardware status

Legend: ✅ works on the phone · 🟡 partly works / needs work · 🔴 not working or disabled · ❔ untested on hardware

| Subsystem | Status | Notes |
|---|---|---|
| eMMC / SD | ✅ | eMMC is visible (HS200, left untouched). SD works; `no-1-8-v` keeps it at 3.3 V high-speed (fixed 2.95 V vqmmc). |
| Touch (Synaptics RMI4) | ✅ | S3708AR registers as an input device. |
| Display (dual DSI, command mode) | ✅ | MDP5 v1.9 + dual DSI + Sergej panel, 1440x2560, fbcon on the panel. 4/4 clean boots. The MDP SMMU's TZ setup is deferred until DRM takes over from the UEFI framebuffer (patch 0034). One `pp done time out` at DRM init remains. |
| CPU clocks | 🟡 | No cpufreq driver yet. At boot `ztboost.ko` (rescue initramfs) raises the A57 cluster to 960 MHz (from 384) and the CCI to 538 MHz (from 269) with the PM8994 S11 rail at 1.0 V; A53 stays at 763 MHz. The SoC lowers the A57 clock itself under multi-core load (4 cores ~862 MHz). The CPU is the 2.0 GHz speed bin (fuse read at `0xFC4BC0B0`); Windows' own frequency/voltage tables (A57 up to 1958 MHz, ceiling 1225 mV) are extracted. DDR/BIMC clock is still unmanaged (DRAM latency ~215 ns). |
| Thermal sensors | ✅ | TSENS v0.1, 16 sensors (patch 0037, calibration from QFPROM): thermal zones for cpu0-7 and gpu. Idle-ish temperatures with software rendering: A53 62-67 °C, A57 65-77 °C. Redundancy fuse region not handled. |
| ztOS userland | 🟡 | Root boots from SD partition 2 (systemd up, SSH as `zerone`, ztOSIAL services). ztMETRO renders on the GPU through Mesa `freedreno` (`/etc/ztos/session.env`), smooth at 60 fps. Before the GPU: llvmpipe, 3-9 fps. |
| GPU (Adreno 430) | ✅ | `drm/msm` + Mesa `freedreno` (`FD430`, OpenGL ES 3.0). Patch 0038: the GPU SMMU's global space (GR0) belongs to TrustZone, so no access before the first GX/CX power-up and `restore_sec_cfg(18)`. Root cause and path: [`docs/research/en/GPU_DRIVER_ANALYSIS.md`](docs/research/en/GPU_DRIVER_ANALYSIS.md). Firmware `a420_pm4/pfp.fw` from linux-firmware. Open: GPU cooling device, devfreq, `vdd`/`vddcx` regulators. |
| Camera | 🔴 | CAMSS and the VFE/Venus SMMUs are disabled in DT for the same SMMU reason; CCI probes. The CCS driver did not work with the sensors before. |
| USB gadget (rescue/SSH) | ✅ | DWC3 + HS PHY (TCSR routing + LK QSCRATCH values). Windows sees ACM + RNDIS, SSH works. |
| Charger / fuel gauge | 🟡 | SMBCHG and PMI8994 FG probe (battery ID is read); init runs in the background (one SRAM grant, ~1 s); capacity, voltage, current and temperature read correctly. |
| Wi-Fi / Bluetooth | 🔴 | PCIe1 (Wi-Fi) not wired in DT; the LineageOS cityman port needs a `GCC_PCIE_1_BCR` reset and the Android QCA6174 `qwlan30.bin`. BT version read times out. |
| Audio | 🟡 | No sound card yet: `msm-snd` deferred. The signed ADSP image is the stock FFU's own `System32\qcadsp8994.mbn` (same hash as the LineageOS cityman port uses); it must be taken from the user's own phone. |
| Modem / SIM | 🔴 | ADSP loads, modem does not (per upstream notes). |
| Sensors, haptics, NFC | 🟡 | Hall, keys, barometer, magnetometer work; MPU6050 / light sensor do not answer. NFC is disabled in DT (without the back cover polarity detection blocks for 33 s). |

## Roadmap

- [x] Feasibility study and ztOS architecture review
- [x] Offline extraction and analysis of the stock FFU (ACPI, firmware inventory, panel, camera)
- [x] Merge fork `master` + `camss` unique parts onto one tree; kernel + cityman DTB build
- [x] Enable `usb3` + UC120 → USB gadget rescue shell (SSH works)
- [x] First on-device boot from SD (efifb console)
- [x] Display: msm DRM on the Sergej panel, boot-time SMMU reset fixed
- [x] Boot time: init at ~3 s (fuel gauge init moved off the boot path)
- [x] ztOS root filesystem on the SD card boots (software rendering)
- [x] A57 / CCI clocks raised at boot; CPU speed bin and Windows frequency/voltage tables found
- [x] TSENS temperature sensors (patch 0037)
- [ ] Remaining display warnings
- [x] GPU: comes up with patch 0038 (next: `glmark2-es2`, devfreq, cooling)
- [ ] Gradual A57 raise with temperature monitoring, A53 above 787 MHz, DDR/BIMC clock, a real cpufreq driver
- [ ] `ztconflo` `uefi-esp` flash method + ztOSIAL manifest
- [ ] Wi-Fi, audio, charging, sensors
- [ ] Camera: CCS sensor bring-up, then AF/OIS/flash, libcamera
- [ ] Generic flashable image / installer

## Repository layout

```
kernel/            Patch series 0001-0037 on top of Android4Lumia950/linux master (0029/0030 add and revert, 0033 is a temporary debug patch kept as history), kernel .config, built DTBs
rescue/            Rescue initramfs (init, USB gadget, SSH, SD logger, boot-time CPU/CCI boost), GRUB menu (FULL DT rescue, SAFE DT rescue, ztOS (default), Windows, previous-kernel fallback), SD/ESP scripts
rescue/tools/      Debug and boot modules: ztboost.c (boot-time A57/CCI clocks), ztcpu.c / ztcci.c (clock experiments), ztfuse.c / ztfuse2.c (speed bin and TSENS calibration fuses), ztts.c (manual TSENS read), ztspmi.c (PM8994 buck dump), ztdt.c (enable a DT node at runtime), ztsec.c (TZ secure page-table SCM calls by hand)
docs/research/     Findings (bring-up log, hardware gaps, GPU/camera report, fork analysis, ACPI device map, camera config); Turkish originals plus English versions in `docs/research/en/`
```

Start with [`docs/research/MDP_SMMU_RESET.md`](docs/research/MDP_SMMU_RESET.md), [`docs/research/PORTING_GAPS.md`](docs/research/PORTING_GAPS.md) and [`docs/research/SOURCES.md`](docs/research/SOURCES.md). Every research note has a Turkish original and an English version (`docs/research/en/`).

## Building the kernel

```sh
git clone https://github.com/Android4Lumia950/linux && cd linux
git checkout master
git am /path/to/ztos-950xl/kernel/00*.patch
cp /path/to/ztos-950xl/kernel/ztos-lumia.config .config   # NR_CPUS=8, DRM_MSM=y, QCOM_WDT=y, tracing on
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j"$(nproc)" Image qcom/msm8994-msft-lumia-octagon-cityman.dtb
```

## Safety

- Take a **full eMMC backup** before doing anything.
- Never touch the EFS, MODEM, SBL, TZ or RPM partitions.
- Booting from SD keeps Windows intact, but a bad kernel can still hang the phone; keep a way to enter flashing mode.

## Legal

- Analysis was done on firmware from the author's own device, for interoperability.
- **No Microsoft / Qualcomm / Lattice firmware or binaries are included or will be.** Anything proprietary must be extracted at install time from the phone itself.
- Kernel patches are GPL-2.0-only (same as the Linux kernel). Research notes are free to reuse with attribution.

## Credits

- [Zerone Laboratories](https://github.com/Zerone-Laboratories) — ztOS / ztMETRO
- [Android4Lumia950](https://github.com/Android4Lumia950) and contributors — Lumia 950 / 950 XL mainline kernel work this builds on (display, GPU, CAMSS, UC120, SMBCHG, audio)
- The Sony/LineageOS MSM8994 3.10 kernels — the downstream SMMU and MDSS code that explained the boot-time reset
- giannoccarol (Lorenzogianno) — LineageOS 23.2 cityman port and its documentation; Empyreal96, wiki.nura.eco and Davitekk — notes and USB PHY values. Full list: [`docs/research/SOURCES.md`](docs/research/SOURCES.md)
- The LumiaWOA / Lumia950XLPkg community — UEFI, and years of reverse-engineering knowledge
- Linux, Mesa/freedreno and the postmarketOS community

## Contributing

Issues and PRs are welcome — especially hardware test reports. If you have a Lumia 950 XL, 950 or a serial/USB-debug setup, open an issue.
