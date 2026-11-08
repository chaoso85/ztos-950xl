<div align="center">

# ztOS-950XL

**Bringing [ztOS](https://github.com/Zerone-Laboratories/ztOS) (Linux + the ztMETRO shell) to the Microsoft Lumia 950 XL**

![status](https://img.shields.io/badge/status-early%20research-orange)
![device](https://img.shields.io/badge/device-Lumia%20950%20XL%20(cityman)-blue)
![soc](https://img.shields.io/badge/SoC-Snapdragon%20810%20(MSM8994)-red)
![kernel](https://img.shields.io/badge/kernel-mainline%20based-success)
![boot](https://img.shields.io/badge/boot-UEFI%20%2B%20SD%20card-lightgrey)

[English](README.md) · [Türkçe](README.tr.md)

</div>

> **Status: early bring-up.** The kernel boots on the phone from an SD card; a rescue shell over USB (SSH) and the SD card work. Display, GPU, camera, audio and Wi-Fi do not work yet. Details: [`docs/research/BRINGUP_LOG.md`](docs/research/BRINGUP_LOG.md) (Turkish). Do not flash anything from this repo to your only device without a full eMMC backup.

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
                 │  GRUB on the SD (\EFI\BOOT\BOOTAA64.EFI)
                 ▼
           Linux kernel (mainline + Lumia patches) + cityman DTB   (acpi=no, device tree)
                 │
                 ▼
           ztOS GSI (ztMETRO)  +  device layer (/vendor/ztos/manifest.toml, ztOSIAL)
```

## Hardware status

Legend: ✅ builds / data ready · 🟡 code exists upstream-in-fork, needs cleanup · 🔴 not started · ❔ untested on hardware

| Subsystem | Status | Notes |
|---|---|---|
| eMMC / SD | ✅ | eMMC is visible (HS200, left untouched). SD works; `no-1-8-v` keeps it at 3.3 V high-speed (fixed 2.95 V vqmmc). |
| Touch (Synaptics RMI4) | ✅ | S3708AR registers as an input device. |
| Display (dual DSI, command mode) | 🟡 | MDP5 v1.9, DSI and panel `sergej` bind but the panel stays dark: pixel/byte clocks are reprogrammed while the DSI PHY PLL is not running, `pp done time out`. |
| GPU (Adreno 430) | 🟡 | `drm/msm` binds; `a420_pm4.fw` missing, `oxili_gfx3d_clk` stuck off (iommu resume, GX power domain). |
| Camera | 🟡 | CAMSS + CCI probe; the generic CCS driver does not work with the sensors (dummy supplies, CCI timeout). |
| USB gadget (rescue/SSH) | ✅ | DWC3 + HS PHY (TCSR routing + LK QSCRATCH values). Windows sees ACM + RNDIS, SSH works. |
| Charger / fuel gauge | 🟡 | SMBCHG and PMI8994 FG probe (battery ID is read); FG probe takes ~20 s. |
| Wi-Fi / Bluetooth | 🔴 | PCIe RP1 (Wi-Fi) not wired in DT; BT ROME/QCA setup command times out. |
| Audio | 🟡 | No sound card: ADSP firmware (`qcadsp8994.mbn`) missing, `msm-snd` deferred. |
| Modem / SIM | 🔴 | ADSP loads, modem does not (per upstream notes). |
| Sensors, haptics, NFC | 🟡 | Hall, keys, barometer, magnetometer work; MPU6050 / light sensor do not answer. NFC is disabled in DT (without the back cover polarity detection blocks for 33 s). |

## Roadmap

- [x] Feasibility study and ztOS architecture review
- [x] Offline extraction and analysis of the stock FFU (ACPI, firmware inventory, panel, camera)
- [x] Merge fork `master` + `camss` unique parts onto one tree; kernel + cityman DTB build
- [x] Enable `usb3` + UC120 → USB gadget rescue shell (SSH works)
- [x] First on-device boot from SD (efifb console)
- [ ] Display + GPU (current work): remove workarounds, proper runtime PM, `kmscube` / `glmark2-es2`
- [ ] ztOS GSI rootfs + `ztconflo` `uefi-esp` flash method + ztOSIAL manifest
- [ ] Wi-Fi, audio, charging, sensors
- [ ] Camera: CCS sensor bring-up, then AF/OIS/flash, libcamera
- [ ] Generic flashable image / installer

## Repository layout

```
kernel/            Patch series (0001-0017) on top of Android4Lumia950/linux master, kernel .config, built DTBs
rescue/            Rescue initramfs (init, USB gadget, SSH), GRUB menu, SD/ESP preparation scripts
docs/research/     Findings (hardware gaps, GPU/camera report, fork analysis, ACPI device map, camera config)
```

Start with [`docs/research/PORTING_GAPS.md`](docs/research/PORTING_GAPS.md) and [`docs/research/GPU_CAMERA_REPORT.md`](docs/research/GPU_CAMERA_REPORT.md). Most research notes are currently written in Turkish.

## Building the kernel

```sh
git clone https://github.com/Android4Lumia950/linux && cd linux
git checkout master
git am /path/to/ztos-950xl/kernel/00*.patch
cp /path/to/ztos-950xl/kernel/ztos-lumia.config .config   # NR_CPUS=8, tracing on
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
- The LumiaWOA / Lumia950XLPkg community — UEFI, and years of reverse-engineering knowledge
- Linux, Mesa/freedreno and the postmarketOS community

## Contributing

Issues and PRs are welcome — especially hardware test reports. If you have a Lumia 950 XL, 950 or a serial/USB-debug setup, open an issue.
