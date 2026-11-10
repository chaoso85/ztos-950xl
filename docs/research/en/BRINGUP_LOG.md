# On-device bring-up log (2026-11-07 … 2026-11-10)

Türkçe sürüm: [`../BRINGUP_LOG.md`](../BRINGUP_LOG.md)

This document collects the **verified** findings from running the kernel on the real phone (Lumia 950 XL, cityman) for the first time. The earlier research notes (PORTING_GAPS, FINDINGS_feasibility) were offline analysis only; everything here was measured on the device.

## 1. Boot chain (working)

```
stock UEFI (eMMC) -> rEFInd -> GRUB arm64-efi (SD: \EFI\BOOT\BOOTAA64.EFI) -> Linux 7.3-rc2 (Android4Lumia950 base) + initramfs
```

- Windows (eMMC) is unchanged; the "Windows (eMMC)" GRUB entry chain-loads `bootmgfw_real.efi`.
- GRUB has no `loadenv/save_env` modules, so a one-shot boot choice is not possible; an entry is picked by hand from the 7 s menu.
- Entries (`rescue/grub.cfg`, state of 2026-11-10, `set default=2`): 0) FULL DT rescue (msm DRM panel, CPU boost on), 1) SAFE DT rescue (MDSS, GPU, camera and SMMUs disabled, UEFI framebuffer; `ztos.noboost` skips the CPU boost), 2) ztOS (`ztos.root=LABEL=ZR_ARCH`, second SD partition; default), 3) Windows (eMMC), 4) previous kernel fallback (`Image.prev`, before TSENS). Earlier changes are in the sections below.

## 2. USB gadget: rescue shell (verified)

Windows sees an ACM (COM port) + RNDIS adapter; ping 192.168.77.1 <-> 192.168.77.2 and key-only SSH (dropbear) work. Fixes (all measured on the device):

| Problem | Cause | Fix |
|---|---|---|
| Port was not routed to DWC3 | TCSR 0xfd4ab000 | write 1 (patch 0014) |
| Windows enumeration hung | LK's QSCRATCH values were missing | re-written through `qcom,lk-wrapper-regs` (patch 0015; values from the Davitekk/ztOS talkman port, not verified one by one for cityman) |
| Code 10 | Empty serial number: the configfs attribute was written as `serialNumber` | lower-case `serialnumber` |
| RNDIS did not match | Microsoft OS descriptors are cached per VID/PID/bcdDevice | change `bcdDevice` |
| RNDIS driver would not start | ACM was bound first | RNDIS first (interface 0), ACM second |

There is no Windows driver for ECM. The IP on the Windows side is assigned by hand (`phonenet.cmd`).

## 3. SD card

`sdhc2` brought the card up as UHS-I DDR50, with a `mmc2: Timeout waiting for hardware interrupt` (CMD6, ADMA Err 3) every ~60 s and a fall back to 3.3 V HS. Cause: `vqmmc-supply` is a fixed ~2.95 V (l13a) and cannot switch to 1.8 V. Fix: `no-1-8-v` (patch 0016).

The rescue init mounts the SD `rw,sync,flush`. A hard reset while logs are being written can corrupt the FAT root directory (it happened once); hence `/sbin/safe-reboot` (stops the logging jobs, syncs, unmounts or remounts `ro`, and only then resets; it refuses to reset if the SD is still rw). "Volume was not properly unmounted" is still printed on every boot; cause unknown.

## 4. Full-DT (display/GPU) boot-log audit

The 3799-line `dmesg` (initcall_debug + `keep_bootcon`) was read from start to end; problems that do not say "error" were listed too. Note: `keep_bootcon` + efifb slows the boot down ~40x, so early timestamps are inflated; multi-second waits are real.

### 4.1 Display and GPU

| Finding | Meaning |
|---|---|
| `pclk0/pclk1/byte0/byte1_clk_src: rcg didn't update its configuration` (8 WARN) | The mmcc RCGs are programmed while DSI opens, but their parent, the DSI PHY PLL, is not running. `msm_mdp: pp done time out, lm=2` (continuous) most likely follows from this |
| `fb0: sys_imageblit / sys_fillrect: framebuffer is not in virtual address space` | fbcon switching to the msm fbdev layer with a broken mapping |
| `qcom/a420_pm4.fw` missing | the sysfs fallback waits ~61 s, then the GPU still does not start |
| `oxili_gfx3d_clk status stuck at 'off'` (7 WARN) + `Failed to enable clk '(null)': -16` | the GPU iommu `qcom_iommu_resume` tries to enable this clock before the GX power domain is on; iommu probe takes ~28 s |
| DSI `lab_reg`/`ibb_reg`, adreno `vdd`/`vddcx` dummy regulators | undefined in the DT |
| `Couldn't register GPU cooling device` | no thermal link in the DT |

### 4.2 Waits that delay the boot without saying "error"

| Wait | Cause |
|---|---|
| ~33 s | `pn544` NFC cannot find the `nfc_en` polarity (back cover = antenna not fitted); it blocks the I2C controller probe. Disabled in the DT (patch 0017) |
| ~19.8 s | fuel gauge probe (battery ID 325 kΩ, expected 330 ±15 %: healthy) |
| ~8.3 s | RPM regulators (`regulators-0`); RPM opens late through `deferred_probe_initcall`; all are "Bringing 0uV into ..." |
| ~5.9 s | `tsl2772` (dummy supply, -110) |
| ~3.3 s | vadc + spurious `spurious spmi irq received for apid=158` WARN |
| 60 s | `qcadsp8994.mbn` missing (finally -110); `regulatory.db` missing |

### 4.3 Hardware missing from, or wrong in, the DT

- `si470x 5-0011`: DeviceID/ChipID 0x0000 but probe "succeeds" (no chip there)
- `fan53555 5-006c`: `Failed to get chip ID`
- `ad714x_captouch 4-002c`: `IRQ not configured` (-22)
- `inv-mpu6050 2-0068`: whoami 0x00, expected 0x70
- `ccs 6-0020 / 6-006c / 7-0020`: dummy supply, registers cannot be written, CCI timeout; the generic CCS driver does not fit these sensors. mmcc `sync_state()` waits for these three
- `i2c_qup f9967000`: no tx DMA channel (PIO)

### 4.4 Kernel and power

- `Number of cores (8) exceeds configured maximum of 6`: the config carried `NR_CPUS=6` from the Lumia 950 (2xA57+4xA53); the 950 XL is 4xA57+4xA53. Raised to 8; the PMU node was split into A53/A57 (patch 0017)
- `psci-cpuidle` returns 19 (-ENODEV): no CPU idle states
- `l2: bypassed regulator has no supply!`, `s2: Bringing 770000uV into 980000uV`
- Bluetooth ROME/QCA6390: command 0xfc00 times out three times (the chip does not answer)
- Audio: `MultiMedia1: error getting cpu dai name`, "No soundcards found" (no ADSP firmware)
- `talkman: camera snapshot reboot armed on gpio-keys` (the camera key can reboot the phone)
- `clk/genpd/regulator: Not disabling unused ...` is intentional (`*_ignore_unused`)
- ramoops records, but UEFI/GRUB overwrite the memory, so nothing survives a reset: pstore is useless

### 4.5 What works

Synaptics S3708AR touch (RMI4), power/resin keys, hall sensor, barometer (zpa2326), magnetometer (ak8975), SD (high speed), internal eMMC visible (`mmcblk1`, 40 partitions; untouched), USB, SMBCHG, MDP5 v1.9 + DSI + panel `sergej` bind.

## 4.6 The to-do list of that day

1. DSI PHY PLL / pixel clock order (pp done time out)
2. GPU iommu clock / power domain + `a420_pm4.fw`, `a420_pfp.fw` into the initramfs
3. DT cleanup: disable si470x, fan53555, ad714x, mpu6050, ccs; tsl2772 supplies
4. Drop `keep_bootcon` to get a realistically timed log

## 5. 2026-11-08: FULL DT boot still had not reached init after 998 s — two separate causes

1. **Real lock-up (caused by 0019):** 0019 removed the `tbu` (OXILI_GFX3D_CLK) clock from the GPU iommu. The reference
   `qcom_iommu_resume()` relies on `clk_bulk_prepare_enable()` failing because of that clock at bind time ("clk_bulk still fails"); when it fails it returns without touching the SMMU. Once tbu was gone the enable succeeded, the reset_ns/BFB writes were made while GX was off, and CPU5 locked up on the bus: RCU stall right after the RPM regulator probe (139.7 s), init never started. 0020 put tbu back; the `stuck at 'off'` warning is the expected, harmless consequence.
2. **Slowness:** `keep_bootcon` keeps the efifb boot console open; ~80 ms per line (normally ~1 ms). The old FULL DT boot (#12) reached init only after 312 s. Removed from the FULL DT entry, with a warning note in grub.cfg; FULL DT became the default entry.

SD layout: `\ztos\{Image, rescue.cpio.gz, grub.cfg, msm8994-msft-lumia-octagon-cityman.dtb, logs}`; `update_sd.ps1` writes them, verifies hashes and the grub.cfg references, and deletes old files.

## 6. 2026-11-09: first clean FULL DT boot (build #14 + 0020 DTB, no keep_bootcon)

- 8 CPUs, init 32.3 s, USB/SSH 34.6 s, SD logging works. **The display works with msm DRM at 1440x2560** (DSI-1 connected/enabled, fb0 = msmdrmfb, DSI0 VCO 1.446 GHz, pclk 120.49 MHz). The a420 firmware was loaded from the initramfs.
- Most of the boot time: fuel gauge probe 19.7 s (3.0 → 20.7 s), deferred probe 8.3 s, RPM regulators 2.3 s, GPU iommu 2.3 s, DSI PHY 2.8 s (while warning traces were printed), panel 1.0 s.
- Side effect of 0018: while orphan RCGs are re-parented at DSI PHY clock-provider registration, `__clk_set_parent_before()` calls `clk_core_prepare_enable()` for CLK_OPS_PARENT_ENABLE; the PHY is not on yet so the VCO prepare fails, the error is ignored, and `_after()` disable/unprepare yields 6 "dsi0pll(byte) already disabled/unprepared" WARNs. Counters stay at 0 (harmless) but it should be fixed.
- Others: `pp done time out, lm=2` (once), `fb0: ... not in virtual address space` (the console works anyway), ad714x "platform data doesn't exist" -22 (the driver has no DT support → node should be disabled), spurious SPMI irq apid=158, BT QCA 0xfc00 timeout ×4, qcadsp8994.mbn missing → no sound card, regulatory.db missing, psci-cpuidle -ENODEV, `l2: bypassed regulator has no supply`, i2c_qup f9967000 no tx DMA, dummy rmi4 vio / zpa2326 vref / DSI lab/ibb / adreno vdd,vddcx.
- The boot before this one restarted by itself; no `dmesg-prev.txt` (it never reached init) and pstore is empty (ramoops memory is not preserved across the restart). Cause unknown; the camera key can also trigger a reboot (talkman).

## 7. 2026-11-09: 0021/0022 and a note on the ADSP firmware

- 0021 (clk core): skip the CLK_OPS_PARENT_ENABLE dance when re-parenting orphans → for the 6 "already disabled" WARNs of 0018.
- 0022: the ad7147 (sideinteraction) node is disabled; the driver has no DT support.
- Backup on the SD: `Image.prev` + `msm8994-msft-lumia-octagon-cityman.prev.dtb` ("PREVIOUS build" GRUB entry) = the last working build.
- ADSP: `qcadsp8994.mbn` is Windows' own signed file (FFU: `mainos/Windows/System32`, 10 149 719 bytes, combined 32-bit ELF, 19 segments, 0x0c800000–0x0e000000 = 24 MiB, segments relocatable). The DT's `adsp_mem` (0x0ca00000, 24 MiB) matches in size; `firmware-name` already equals the Windows name. The file is signed (TZ verifies it) → we cannot write or modify it, only take it from the user's own FFU; it is not put into the repository. Audio additionally needs WCD9330 (SLIMbus), TAS2552 and the q6 APR/AFE path; `.acdb` calibration files are not used in mainline.

## 8. 2026-11-09: every entry resets at ~30 s (including yesterday's working build)

- Windows boots normally, battery 70 %. There is no `dmesg-prev.txt` on the SD → none of today's boots reached init (32 s).
- It also resets with yesterday's working Image/DTB, so the cause is not the build; something depends on hardware/firmware state.
- Hypothesis (unverified): the APSS watchdog (f9017000) that the firmware sometimes leaves armed. CONFIG_QCOM_WDT was off (also off in the reference mmo_defconfig), so nobody fed it. 0023: QCOM_WDT on + EN/STS/BARK/BITE logging at probe; PMIC reset-reason (PON/WARM/POFF/SOFT) logging in qcom-pon probe.

## 9. 2026-11-09: reset point found from a video → GPU + GPU SMMU disabled (0024)

- Video (28 s): the last line read is 26.649 s `platform fdb00000.gpu: Adding to iommu group 0`, then the reset happens while a new `oxili_gfx3d_clk stuck at 'off'` WARN is being printed. In yesterday's working boot the same spot was at 26.18 s and passed.
- `qcom_iommu_attach_dev()` ignores the `pm_runtime_get_sync()` failure and programs the GPU SMMU context bank while GX is off; the result depends on the hardware state (passed yesterday, reset today, black screen + freeze on the PREVIOUS entry).
- SAFE DT (GPU + GPU SMMU disabled) never had this problem. 0024 disables &gpu and &gpu_iommu in FULL DT; the display (MDP/DSI) does not need the GPU. The GPU was not working anyway (no GX rail defined).
- SD: PREVIOUS entry = yesterday's Image + the new (GPU disabled) DTB.

## 10. 2026-11-10: FULL DT with the GPU disabled still resets at ~25.07 s → CAMSS/VFE/Venus SMMUs disabled (0025)

- Video (last 7 s at 10 fps + last 1.4 s at 30 fps): no gfx3d WARN (0024 worked). Last lines: venus SMMU (fdc84000) contexts + "returned 0 after 95671 usecs", then VFE SMMU contexts (fda4c000/fda4d000) at 25.068 s → reset. In yesterday's successful log `fda0ac00.camss: Adding to iommu group 1` (attach) follows right after this point.
- Same class of problem: the SMMU attach programs the context bank while the block is unpowered. SAFE DT had all of them disabled.
- 0025: &camss, &vfe_iommu, &venus_iommu disabled. &mdp_iommu stays on for the display; if that resets too, it is the next suspect.

## 11. 2026-11-10: FULL DT with GPU + CAMSS/VFE/Venus SMMU disabled boots (#17, DTB 0025)

- init 25.37 s, USB/SSH 27.9 s; the display works with msm DRM ("no GPU device was found" is expected). The DSI PHY no longer takes 2.8 s (no WARN after 0021). No ad714x line (0022).
- Watchdog: `EN=0x0 STS=0x0` → the firmware did not leave the APSS watchdog armed; the watchdog hypothesis was WRONG.
- PMIC: `pon a0 warm 00 00 poff 08 00 soft 00 00`. By the downstream qpnp-power-on table POFF bit3 = GP1 (Keypad_Reset1, forced power-off with keys); no warm-reset reason. The previous cycle ended with the user's key power-off.
- Remaining: fuel gauge 19.3 s, `pp done time out, lm=2` ×2, fb0 "not in virtual address space" (imageblit/fillrect), spurious SPMI irq apid=158, BT QCA timeout ×4, qcadsp8994.mbn missing, sound card deferred, regulatory.db missing, psci-cpuidle -ENODEV, l2 "bypassed regulator has no supply", i2c_qup f9967000 no tx DMA, dummy rmi4 vio / zpa2326 vref / DSI lab/ibb, simple-pm-bus devlink error, qseecom skipped.
- USB: the cable was not plugged in during boot (smbchg-usb=0); configured when plugged at 84 s; unplug/plug works fine.

## 12. 2026-11-10: panel black with msm DRM; the MDP SMMU does not work

- With the 0025 DTB, 5 consecutive boots reached init + USB (PMIC: PS_HOLD, no unexpected reset), but the panel is black: DRM "connected/enabled/DPMS On", clocks correct, MDP/DSI interrupts increasing, yet `pp done time out, lm=2` on every frame; one boot had `fd9d4000.iommu-ctx: timeout waiting for TLB SYNC` and a 5.5 s panel probe.
- Difference from #14 (where the display worked): the GPU/CAMSS/VFE/Venus SMMUs had been probed. The Venus SMMU is the only instance with a secure context → `qcom_iommu_sec_ptbl_init()` ran only in its probe.
- Experiment: a DTB re-enabling only the Venus SMMU → display lost at ~25 s, no reset, no USB (freeze before init). So the MDP path with ptbl init is not reliable either (#14 is the only successful sample; the same structure gave reset/freeze in other attempts).
- Decision: the default entry is SAFE DT (MDSS + all SMMUs disabled, UEFI framebuffer, fuel gauge/BT disabled). The msm DRM entry stays as "EXPERIMENTAL". In 7.3 msm KMS does not work without an IOMMU ("no IOMMU, bailing out").
- Open work: MDP SMMU (secure-id 1) — restore_sec_cfg/ptbl order, TLB sync, stall-on-fault; compare with downstream 3.10.

## 13. 2026-11-10: external source survey — notes (input for the next step)

### Downstream 3.10 (LineageOS android_kernel_sony_msm8994 cm-14.1; oneplus_msm8994 has the same base)
- `msm8994-iommu.dtsi` MDP SMMU: `qcom,msm-smmu-v1`, reg 0xfd9cc000 **0x10000**, `qcom,iommu-secure-id = <1>`, ctx `mdp_0`@fd9d4000 (SID 0, NS), `mdp_1`@fd9d5000 and `mdp_2`@fd9d6000 secure; `vdd-supply = gdsc_mdss`; bus vote `mdp_ebi` 22→512 (0 / 1000 KBps); BFB: 0x2000=0x3, 0x204c=0x7fffff, 0x2060=0x1777, … (19 registers). Our `msm8994_mdp_bfb` deliberately skips the 0x2000=0x3 register (reference comment: "impl-def bias removed").
- `msm_iommu-v1.c` attach order: GDSC + bus vote + clocks → (if secure) `scm restore_sec_cfg(sec_id, ctx)` → BFB → halt → program ctx (reset ctx, TTBR0, ACTLR BPRC*, ASIDPNE, **TLBIASID + TLBSYNC**, then SCTLR.M) → resume → TLBIASID + sync again. Our `qcom_iommu_program_ctx()` does no TLB invalidate/sync before setting the M bit.
- **Key point in `mdss_mdp.c`:** "delay iommu attach until continous splash screen has finished handoff, as it may still be working with phys addr". Alternative path (`mdss_mdp_splash_logo.c`): map the splash memory 1:1 (iova = phys) in the iommu and attach like that. In our case the MDP SMMU comes up while UEFI is still scanning the framebuffer from its physical address (`fd901000.mdp: Adding to iommu group 0` → default DMA domain attach) → the most likely explanation of the reset/freeze/TLB SYNC timeout/`pp done time out` picture.
- Candidate fixes: (a) `get_resv_regions` support in `qcom_iommu` (of_iommu `iommu-addresses` 1:1) + mark the efifb region in the DT with `iommu-addresses = <&mdss_mdp …>`; (b) stop the MDP INTF timing engines before the SMMU M bit is set (msm `mdp5_kms_init` does this, but AFTER the iommu group attach); (c) the 0x2000 BFB + TLBIASID/sync differences.

### wiki.nura.eco microsoft-cityman (saved copy: wph\webpage_nura)
- State of 19 September 2026 (for Android4Lumia950/linux): Wi-Fi & Bluetooth work, GPU works but needs improvement, QC charging (~9 W), battery status, main speaker, some sensors. Table: display/touch Works, USB networking Broken.
- Older note: ath10k (PCIe QCA6174) used to lock up the system; BT and Wi-Fi need QCA6174 rampatch/TLV blobs.
- Our base = Android4Lumia950/linux master (2026-09-10, eb871dc) — there is no newer master. Side branches: `iommu`, `mdss` (2026-08, older states already merged into master), `camss` (2026-08-29), `msm8994-6.x` (2024-12: "ADSP loads… MDSS hangs the kernel"), `djokcho` (2025-03). Wi-Fi/BT/GPU/audio additionally need firmware + config.

### Empyreal96 (guide + packed-lumia-950xl-kernels)
- Display is efifb only (`CONFIG_DRM_MSM` off, `video=efifb`); no MDSS/IOMMU in the DTB.
- BT firmware: `qca/rampatch_00440302.bin` (sha256 998d8d1f…), `qca/nvm_00440302.bin` (8fbe6d65…). Wi-Fi: PCIe1 + ath10k.
- `CONFIG_QCOM_WDT=y` in the 5.8-rc3 config.

### Others
- openSUSE Hack Week: angler/bullhead mainline; cont_splash_mem mapping fixes (v6.4, fe88480a6be9, c85c8a992794).
- somainline msm8994 page, AOSP hardware/qcom/msm8994 (userspace HAL), UniXNU-lumia950 (XNU skeleton): nothing for the kernel side.

## 14. 2026-11-10: 0026 — MDP SMMU default domain is identity (bypass)

- Equivalent of the downstream `mdss_iommu_ctrl()` logic: when the MDP device is added to the iommu group (while UEFI is still scanning from the physical address) it gets an identity domain instead of the translating DMA domain; the context bank is not touched. msm DRM attaches its own paging domain in `msm_kms_init_vm()`, after `mdp5_kms_init()` has stopped the timing engines.
- Test: GRUB entry 2 ("FULL DT, msm DRM on (TEST…)"), Image 76509d8c242f, DTB 927c2b4a77bb (0025: GPU/CAMSS/VFE/Venus disabled).

## 15. 2026-11-10: LineageOS 23.2 (Android 16) cityman port — giannoccarol (XDA: Lorenzogianno)

- Kernel: `giannoccarol/android_kernel_mmo_msm8994` branch `lineage-23-cityman` (3.10.108, Android4Lumia950 MMO kernel + cityman; last commit 2026-11-08). The boot chain is **LK** (Android4Lumia950 installer, Windows is erased) — different from our UEFI→GRUB chain; LK brings the display up itself and hands it to the downstream mdss.
- Working (first message, 2026-11-08 build): display + GPU, AOD, touch, Wi-Fi (QCA6174), BT (A2DP), LTE/SMS/calls (CSFB), speaker/headset/microphone, NFC, sensors, deep sleep, rear/front camera, flash, Venus hardware video, USB ADB/OTG, 3 A USB-C charging, **UC120 PD + DisplayPort alt mode** (USB-C→HDMI 1080p60). No GPS, SELinux permissive.
- Files: `arch/arm64/boot/dts/mmo/msm8994-mmo-cityman.dts`, `common/*.dtsi` (mdss, regulator, connectivity…), `panels/dsi-panel-sergej-wqhd-dualdsi-cmd.dtsi`, `drivers/misc/uc120.c`, `sound/soc/codecs/tas2552.c`, `Documentation/arm64/cityman-{adsp,venus,wifi,ril,credits}.txt`.
- Panel: `vddio-supply = pm8994_s4`, `qcom,dsi-panel-bias-vreg` + PMI8994 `labibb` "oled" mode (we have dummy lab/ibb).
- **ADSP:** the signed cityman image `qcadsp8994.mbn` 10 149 719 B, SHA-256 3e101699…a835e, HW_ID 009400E100450000 (Nokia trustroot). **The `System32\qcadsp8994.mbn` in our FFU has the identical hash.** The Angler/other images are rejected with PAS_INIT -2.
- **Venus:** `qcvss8994.mbn` 1 002 800 B, SHA-256 7db914db…0182 — **the file in the FFU is identical.**
- **Wi-Fi:** the PCIe1 endpoint read 0xffffffff; before the first setup `GCC_PCIE_1_BCR` (GCC+0x1b40) assert/deassert → 168c:003e. `qcom,ep-wakeirq` restored. Firmware: Android QCA6174 hw3 `qwlan30.bin` (750 788 B, f84fd11d…); the Windows `qwlan30.bin` in the FFU (1 068 368 B, 3b7e5f82…) **does not work**. The board calibration `bdwlan30.bin` is the device's own file.
- Modem: MBA 0x06e80000, metadata 0x06f80000; modem 0x07000000, peripheral 0x0ca00000 (31 MiB, ADSP ~24 + Venus ~5), ramoops 0x0f200000.

## 16. 2026-11-10: 0026 test + 0027 (TZ secure page-table pool)

- With 0026 (MDP SMMU identity): no reset/freeze, no `TLB SYNC` timeout, init 26.0 s, USB/SSH works. But the panel is still black: `pp done time out, lm=2` ×2, no SMMU fault message.
- Difference from the only boot where the display worked (#14): there the Venus SMMU probed before the MDP SMMU and ran `qcom_iommu_sec_ptbl_init()` ("iommu sec: pgtable size: 151552"). Since 0025 Venus is off → the pool is never set up, and the MDP SMMU (secure-id 1) is restored by TZ without a pool.
- 0027: the pool is set up (once) for every TZ-managed instance — like downstream `msm_iommu_sec_ptbl_init()`. Image 94982b91da1a, written to the SD over SSH (DTB unchanged: 927c2b4a77bb).
- **Result (0027, Image 94982b91da1a):** on the 2nd boot the panel shows an image with msm DRM; `iommu sec: pgtable size: 151552` is now printed from the MDP SMMU (fd9cc000), `pp done time out` went from 2 to 1, init 25.9 s, USB/SSH works.
- The 1st boot of the same build reset at ~25 s; no log since it did not reach init. On the next boot PMIC: `pon 21 poff 02` = power-off via PS_HOLD (not the user's key; a TZ/software-caused reset). The reset point is still the MDSS/MDP SMMU probe window (24–25.3 s).

## 17. 2026-11-10: 0028 — OCMEM disabled (wrong lead)

- 120 fps video of the resetting boot of 0027: the last lines read were `ocmem fdd00000.sram: 8 ports…` (25.439), `probe of fdd00000.sram … returned 0` (25.456), `l20: Bringing 0uV into 2950000uV` (25.466). In a good boot the MDP SMMU's `iommu sec: pgtable size` line follows right after OCMEM.
- Only the (disabled) GPU uses OCMEM, but its probe turns on VDD_GFX, sets the OCMEMGX clock and calls TZ restore_sec_cfg(OCMEM). 0028 disabled `&ocmem` on cityman. **Result: 2/2 boots reset again** → not the cause. (Leaving it disabled is harmless; it will be re-enabled with the GPU.)

## 18. 2026-11-10: 0029 — secure page-table pool in a no-map region (wrong lead)

- 0028 video: last lines `mmcc … returned 0` (24.316), `qcom-iommu fd9cc000.iommu: iommu sec: pgtable size: 151552` (24.331), `s5: Bringing 0uV …` (24.339).
- Hypothesis: in msm-3.10 a `DMA_ATTR_NO_KERNEL_MAPPING` buffer is removed from the linear map, mainline does not; since TZ locks the pool with an XPU, speculative CPU access is an XPU violation → reset. 0029: a 256 KiB no-map reserved-memory for the MDP SMMU (`memory-region`). **Result: reset again**; the last visible line is now the interconnect probe (24.949) → hypothesis wrong.

## 19. 2026-11-10: modular DRM, a test module and 0030/0031 (wrong lead again)

- After the reset pstore is empty (RAM is not preserved).
- 0029 + `CONFIG_DRM_MSM=m` (the panel driver as a module too): only the MDP SMMU probes at boot. Video: last line `iommu sec: pgtable at 0x5ff60000 (no-map reserved)` (23.834), no `pgtable init done` → "reset inside `PTBL_INIT`" was assumed.
- `rescue/tools/ztsec.c` (test module) on a SAFE boot (quiet system; venus/camss/oxili GDSCs off, MDSS on): PTBL_SIZE 151552, dma pool 0xdd100000, **PTBL_INIT ret 0, no reset**.
- From this, "the timing of the call matters" was concluded: 0030 reverted 0029, 0031 sets the pool up early like downstream (in the `qcom_scm` probe, ~0.46 s) and qcom_iommu skips its own setup. 0031 is correct and permanent but did **not** fix the reset: the last visible line is now the `fd_gdsc` probe (24.925), even before the MDP SMMU.

## 20. 2026-11-10: the last line on screen is not the moment of the reset

- The videos were counted frame by frame: the screen goes from 0.4 s to 24.9 s of kernel time in ~4 s of real time. So fbcon freezes for seconds and then prints the accumulated log; **the last visible line is not where the reset happens**. All the "last line" inferences in 17–19 were misleading because of this.
- The FULL entry was booted without `initcall_debug`: there is no log of that boot on the SD (`dmesg-prev.txt` is a SAFE boot) → the reset is before init (2.5 s in SAFE). The reset is not at a fixed second, but at a fixed point of the boot sequence.

## 21. 2026-11-10: 0032 — PS_HOLD warm reset option (for ramoops; did not help)

- `qcom_pon.ps_hold_warm=1`: PMIC PS_HOLD reset type warm (gen1 PON 0x5a/0x5b, msm-3.10 `qpnp-power-on.c`). The FULL entry logged only to ramoops (`console=ttyMSM0`, so that fbcon causes no delay).
- Result: the reset reason is `warm 02` (a warm reset really happened) but the ramoops region (0x0f200000) is invalid on the new boot → UEFI/DDR does not preserve the contents. The option stays in the kernel, default off.

## 22. 2026-11-10: root cause — TZ `restore_sec_cfg` on the MDP SMMU while UEFI is scanning (0033–0035)

The method changed: take a SAFE boot (always sound) and enable the FULL-only DT nodes one by one **at run time** (`rescue/tools/ztdt.c`, `status = "okay"` through an `of_changeset`; OF reconfig notifiers create the device) while streaming `/dev/kmsg` to the PC over SSH. If it resets, the default GRUB entry falls back to SAFE and the loop continues by itself.

| Node (on in FULL, off in SAFE) | Result |
|---|---|
| `pmic@2/fuel-gauge@4000` | did not bind ("no parent regmap": SPMI children are not created through reconfig), no reset |
| `i2c@f9963000/qpdst900@39` | apds9930 / tsl2772 bound, no reset |
| `cci@fda0c000` | i2c-qcom-cci bound, no reset |
| `iommu@fd9cc000` (MDP SMMU) | **reset, every time** |

The log streamed with 0033 (temporary step markers, 400 ms wait after each step): `PTBL_SIZE` → `PTBL_INIT ret 0` → contexts are created → resume: clocks on → `restore_sec_cfg(1, 0)` is called → **reset**. The caller is the "map after parent resume" in the ctx probe (the parent SMMU is runtime-resumed). At that moment the UEFI framebuffer is still being scanned through MDP from its physical address.

Downstream `mdss_mdp.c` says exactly this: "delay iommu attach until continous splash screen has finished handoff, as it may still be working with phys addr" — in 3.10 `restore_sec_cfg` + BFB are done at attach time (`msm_iommu-v1.c`). 0026 had only deferred the domain attach; the driver's own resume still made the TZ call at boot.

- **0034:** the 8994 MDP SMMU is `hw_deferred`: resume only turns the clocks on; ctx probe does not resume the parent (ioremap does not touch the hardware); `restore_sec_cfg` + BFB are done at the first translating domain attach (msm DRM, after `mdp5_kms_init` has stopped the UEFI scan), normal afterwards.
- Test (SAFE + run time): MDP SMMU turned on → no reset. MDSS turned on + msm DRM loaded → deferred `restore_sec_cfg ret 0`, `[drm] Initialized msm`, `fb0: msmdrmfb`, the phone stays up.
- **0035:** step markers became `dev_dbg`, waits removed; `DRM_MSM` and the panel built in again (Image 8d6fd74eaffd).
- Lesson: do not look for the reset point from fbcon's last line; live log streaming (SSH) + enabling DT nodes at run time is much faster.

## 23. 2026-11-10: FULL DT reliability (with 0034)

- GRUB: FULL DT is now the default, SAFE second (grub.cfg 25c9a4526b7e).
- **4/4 clean boots** (user boot + 3 safe-reboot cycles). Each came back in ~75 s (~40 s shutdown/UEFI/GRUB + ~35 s uptime → no hidden boot in between).
- On every boot: `early pgtable init done` 0.46 s, `[drm] Initialized msm` ~22 s, `msmdrmfb`, init ~22.5 s, console on the panel, no SMMU fault/oops.
- Remaining (not resets): the fuel gauge probe blocks ~18 s (2.3 → 20.4 s); `pp done time out, lm=2` once while DRM opens; `fb0: sys_imageblit: framebuffer is not in virtual address space`; `spmi-pmic-arb.c:786` WARNING (`pmic_arb_chained_irq`); Bluetooth QCA version read timeout; `qcadsp8994.mbn` and `regulatory.db` not in the initramfs.
- Patches: 0029/0030 (add + revert) and 0033 (temporary debugging) stay in the series as history; they will be merged when the series is reduced to a clean one.

## 24. 2026-11-10: the fuel gauge delayed the boot by 19.6 s → 0036

- Cause: probe read the battery ID and then made ~10 separate FG SRAM writes (thermal coefficients, JEITA, cutoff, termination currents, resume SoC, two masked writes); each one re-requested MEMIF access and waited for an FG cycle (~1.5–2 s).
- Reference: downstream `qpnp-fg.c` (giannoccarol `lineage-23-cityman`; LineageOS sony is the same): `fg_hw_init()` runs in the work queue after probe through `delayed_init_work()`; `fg_mem_lock()/fg_mem_release()` keep a single access grant for the whole init.
- 0036: probe only verifies the peripherals and queues the init; the init runs under one grant and the battery is registered afterwards.
- Result (Image 791d69373911): `FG init took 1020 ms`, **init 22.5 s → 3.05 s**, USB/SSH 5.3 s, safe-reboot cycle 75 s → 45 s. `battery`: Charging, 58 %, 3.89 V, 92 mA, 41.5 °C, `microsoft_bvt4d_3340mah`.

## 25. ztOS root filesystem booted from the SD (2026-11-10)

- Davitekk's ztOS installer was built for the `cityman` target (Arch Linux ARM + ztMETRO + ztOSIAL, 10 GiB ext4, label `ZR_ARCH`) and written to the SD card's second partition.
- A one-shot GRUB entry mechanism was tried (`next_entry` in `/ztos/grubenv`); it did not work because GRUB could not write to FAT (section 27); in the end `set default=2` is used.
- With `ztos.root=LABEL=ZR_ARCH` the initramfs did a `switch_root`: systemd `running`, no failed service, SSH works with `zerone`, ztOSIAL and the ztos* services are up.
- The screen was empty: with the GPU disabled, Mesa freedreno could not get a GPU on `msm`, wlroots could not create a renderer, ztmetro exited, greetd hit its start limit. `/etc/ztos/session.env` forced `kms_swrast`/`llvmpipe`; ztMETRO came up at 1440x2560 on `llvmpipe` (GLES 3.2), 6 tiles. CPU ~300 % (software rendering).
- Remaining: GPU, Wi-Fi, audio (ADSP), backlight (`no usable backlight`), BT (QCA version read timeout), growing the root to the whole partition.

## 26. Why the ztOS UI is slow: CPU/memory measurements (2026-11-10)

The screen runs at 3-5 fps, touch is ~2 s late. Measurement (static aarch64 benchmark, with ztMETRO stopped, registers read through `/dev/mem`):

| | A53 (cpu0-3) | A57 (cpu4-7) |
|---|---|---|
| effective clock (dependent add chain) | ~763 MHz | ~381 MHz |
| PLL (`f9015000` / `f9016000`) | L=0x50 (1536 MHz), mux 0x80000004 (PLL0 main, /2) | same L, but PLL user_ctl post-div 0x309 (A53: 0x11f) → half speed |
| L1 latency | 3.9 ns | 10.5 ns |
| RAM latency (64 MB chain) | ~208 ns | ~330 ns |

- There is no CPU clock/cpufreq driver in the kernel: the cores stay at the value UEFI left. zrnperfd knows this and pins everything to cpu0-3 (A53) (`cpuset`).
- RAM latency is 2-3 times the normal value (~100 ns): the memory path (BIMC/CCI) is at the bootloader clock too. CCI PLL L=0x1c = 537.6 MHz.
- The first Python benchmark was misleading: ztMETRO threads ran at `nice -15`, the measurement at `nice 0`.
- Experiments: `WLR_RENDERER=pixman` does not work (ztMETRO uses the GLES2 renderer directly). Opening all 8 cores (zrnperfd stopped, cpuset 0-7) spread the CPU load from 4 to 8 cores.

Reference: MMO downstream `drivers/clk/qcom/clock-cpu-8994.c` (A53/A57/CCI alpha PLL + `a5x-lf/hf-mux`, MUX_OFFSET 0x54, speed tables depend on the efuse speed bin, voltage `apc0/apc1_vreg_corner`). `qcom,cpu-clock-8994` serves both the 8994 and the 8992 (Lumia 950); the 950 lacks two A57s and its DT cpu nodes must differ.
Next: mainline `clk-alpha-pll` + a custom mux/div clock provider + `cpufreq-dt` OPP table (no high frequency until the voltage is settled), then BIMC/CCI votes.

## 27. Reading QFPROM through /dev/mem reset the phone; the grubenv one-shot entry did not work (2026-11-10)

- To find the CPU speed bin, `0xfc4b80b0` and `0xfc4bc000+0x1f0..` (the QFPROM/CPR fuse area) were read through `/dev/mem` from inside ztOS. The phone reset instantly (SSH "closed by remote host", uptime 0). ext4 `recovery complete`, no data loss. **Lesson: QFPROM is not read from user space through /dev/mem**; the speed bin must be read inside the kernel with the `qcom,qfprom` nvmem driver (the TZ-compatible path). (Section 30 later found that the corrected alias 0xfc4bc0b0 is safe and the raw aliases were the problem.)
- GRUB `save_env` could not write to FAT: after the reset `next_entry=2` was still in place and the phone booted ztOS again. The one-shot entry mechanism does not work; ztOS is the default on every boot for now. Rescue entries are picked by hand from the menu (7 s). The fix: use `set default=2` in `grub.cfg` and drop the `grubenv` logic.
- In ztOS the `sudo` password for `zerone` is no longer `toor` (`auth could not identify password`); it may have changed after the Welcome app was run.

## 28. A57 clock raised 381 → 765 MHz (plain writel, no SCM)

- Earlier hangs of the A57 mux write were NOT a TZ/XPU protection: my "safe" source was lfmux sel 1, which is PLL1 (PLLA) main and is switched off by the bootloader, so the cluster lost its clock (PS_HOLD reset).
- Reference: downstream `drivers/clk/qcom/clock-cpu-8994.c`, `cpu_clock_8994_init_a57_v2()` (the qcom,cpu-clock-8992 / 8994-v2 path) writes mux 0x54 with plain `writel_relaxed(0x6)` = lfmux sel 3 = AUX / GPLL0 600 MHz. `sanity_check_clock_tree()` documents PLL USER_CTL bits[9:8]: 1 = div 2, 3 = div 4, rate = L * 19.2 MHz / div.
- Working sequence (rescue FULL DT, tools/ztcpu.c): S11 900 -> 1000 mV over SPMI; mux sel 3 (GPLL0); PLL f9016000 USER_CTL 0x309 -> 0x109; mux sel 2 (PLL0 main).
- SCM `qcom_scm_io_readl` of these registers returns -22 on this TZ; not needed.
- Measured with cpubench: cpu4/cpu5 ~765 MHz (was ~381), L1 sweep 2574 MB/s (was ~1290 est.), DRAM stride 2690 MB/s.
- Next: the same on A53 (above 768 MHz needs 1.18 V + mem-acc), CCI/BIMC votes, then a real mainline-style cpufreq driver.

## 29. A57 960 MHz, CCI 538 MHz, applied at boot (rescue/tools/ztboost.c)

- A57 PLL retune works from the safe source (GPLL0 aux, lfmux sel 3): L=90 -> 861 MHz, L=100 -> 956 MHz (single core). The SoC lowers the clock itself with more loaded A57 cores (2 cores 950, 3 cores ~899, 4 cores ~766 in the first run, ~862 after the boot-time setup). Rail S11 held at the downstream CPR ceiling for these steps (1000 mV, msm8992-regulator.dtsi apc1 fuse corner 3).
- CCI: bootloader runs it at 268.8 MHz (CCI PLL L=28 = 537.6 MHz, hfmux 2 = PLL/2). hfmux 3 (PLL direct) doubles it, through AUX as safe source, plain writel works (S8 is already 1000 mV). Effect: L2-miss/CCI latency 57.7 -> 42.4 ns at 2 MB.
- DRAM latency is unchanged (~215 ns) by the CCI step and by an interconnect bandwidth vote (mas_ampss_m0 -> slv_ebi_ch0, 6.4 GB/s; the MDP already votes that). BIMC/DDR clock needs a working RPM path: open item.
- SCM io_readl of these registers returns -22; not needed.
- rescue/init loads /lib/modules/ztboost.ko at boot (skipped with ztos.noboost, set on the SAFE DT entry). Verified: ztboost runs at 2.7 s, cpu5 956 MHz, 4 x A57 862 MHz.
- A53 stays at 763 MHz (above 787 MHz needs the 1180 mV corner on S8, shared with the CCI).

## 30. CPU speed bin and Microsoft's frequency/voltage tables (Windows PEP, stock DSDT)

- Speed bin: the stock DSDT (ffu_extract/plat/ACPI/dsdt.dsl, method with `OperationRegion (FUSE, SystemMemory, 0xFC4B8000, 0x5000)`, `Field QWordAcc { Offset (0x40B0) PTE, 64 }`) reads PTE at 0xFC4B8000 + 0x40B0 = **0xFC4BC0B0** and uses `PTE & 7`: 0 = "2.0GHz", 1 = "1.8GHz". The earlier phone resets came from reading 0xfc4b80b0 (raw alias, as the downstream Android driver does) and 0xfc4bc000 - not from the right address. One readq() of 0xFC4BC0B0 (rescue/tools/ztfuse.c) is safe: PTE 10a1c71c8e4e78b0 -> speed bin 0 = the 2.0 GHz part.
- Windows 10 Mobile's QCpep8994.sys (ffu_extract/mainos/Windows/System32/Drivers) carries the full tables, entries {index, Hz, corner id, ceiling uV, nominal uV, floor uV, -1} at file offsets 0xba138 (A53), 0xba278 and 0xba748 (A57, two bins), 0xba418/0xba648/0xba978 (CCI), 0xba580.
  - A53: 384 .. 1555.2 MHz (600/672/768 MHz at 1000 mV; 864/960 MHz nominal 1115 mV; 1248..1555 MHz nominal 1180 mV, ceiling 1225 mV).
  - A57 (14 steps, the 2.0 GHz bin): 384 480 633.6 | 768 864 960 (1000 mV) | 1248 1344 1440 1536 1632 (nominal 1160 mV, ceiling 1225) | 1728 1824 1958.4 MHz (1225 mV). The other table ends at 1766.4 MHz (the 1.8 GHz bin).
  - CCI: up to 921.6 / 940.8 / 787.2 MHz in three variants; 600 MHz and up needs 1225 mV.
- Ceiling for the highest corner on this SoC is therefore 1225 mV (not 1180 mV as in the Lumia 950 / MSM8992 regulator dtsi).

## 31. TSENS (SoC temperature sensors) for MSM8994

- Only the PMIC die and the battery were visible; the 16 TSENS sensors (cpu0-7, gpu, pop memory) were not exposed.
- Same v0.1 TSENS IP as MSM8974, 16 sensors. Kernel patch 0037: MSM8994 data in tsens-v0_1.c, calibration read from QFPROM corrected alias 0xfc4bc0d0 (three 32-bit words, same alias as the speed bin; verified: cal_sel 3 = two-point, base0 526, base1 833, all offsets 0), DT node tsens@fc4a9000 (TM) / fc4a8000 (SROT), IRQ SPI 184, read-only thermal zones. Sensor numbering from the downstream qcom,sensor-information: 7-10 = cpu0-3 (A53), 13/14/15/6 = cpu4-7 (A57), 12 = gpu, 2 = pop memory.
- Kernel config: CONFIG_NVMEM_QCOM_QFPROM=y (dependency; with it off, `olddefconfig` silently dropped the next one) and CONFIG_QCOM_TSENS=y. The kernel now builds from a clean tree (7.3.0-rc2-sumire-g971297720829); the old "-dirty" string would have broken module loading (vermagic).
- SD: new Image, both DTBs, rescue.cpio.gz (ztboost.ko rebuilt for the new release), grub.cfg with a fifth entry "PREVIOUS kernel" (Image.prev + cityman.prev.dtb).
- Redundancy region (+0x464 ...) of the downstream calibration is not handled yet.
- Result (final kernel 7.3.0-rc2-sumire-g70b1eac4d0ff): the sensors were already running when Linux starts (secure firmware) although the SROT TSENS_EN bit reads 0; mainline init_common refused on that bit ("device not enabled"). New feature flag skip_en_check fixes the probe. Sensors 0..4 read code 0 (not connected, no zone); 5 and 11 are live but have no known owner. Hand-made read (rescue/tools/ztts.c) and the driver agree. First readings (just after a boot with software rendering on all cores): cpu0-3 (A53) 62-67 °C, cpu4-7 (A57) 65-77 °C, gpu 75 °C, PMIC die 63 °C, battery 50 °C. The A57 cluster already sits at ~75 °C at idle-ish load, so clock/voltage increases need a margin check.
