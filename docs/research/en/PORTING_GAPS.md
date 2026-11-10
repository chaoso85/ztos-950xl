# Lumia 950 XL (cityman, RM-1085): hardware descriptions missing for mainline Linux

Türkçe sürüm: [`../PORTING_GAPS.md`](../PORTING_GAPS.md)

Three independent sources were cross-checked:
1. **Stock W10M FFU** (10586.13169): PLAT\ACPI DSDT/SSDT, UEFI DXE drivers, MainOS driver/firmware files
2. **Mainline Linux** (torvalds/linux master): msm8994.dtsi + msm8994-msft-lumia-octagon{.dtsi,-cityman.dts}
3. **Android4Lumia950/android_kernel_mmo_msm8994**: downstream arch/arm64/boot/dts/mmo/* (kept in `downstream_dts/` of the working folder)

## Cross-validation results (data reliability)
| Check | Result |
|---|---|
| Touch: DSDT TSC1 = I2C1 0x4B | same as the mainline DT `rmi4-i2c@4b` |
| Panel timings: UEFI XML (Sergej) vs downstream `dsi-panel-sergej-...dtsi` | HFP 76, HBP 32, HSYNC 16, VFP 4, VBP 2, VSYNC 2, 60 Hz, command mode: **identical** |
| Panel GPIOs: DSDT DISP (GpioInt 0x0A, GpioIo 0x4E) vs downstream mdss.dtsi (te-gpio 10, reset-gpio 78) | **same** (GPIO10 = TE, GPIO78 = reset) |
| Selected panel: downstream msm8994-chi.dtsi imports only Sergej | cityman = Sergej (Duke is also defined in UEFI) |

## What is enabled in mainline cityman today (consistent with the stock data)
eMMC (sdhc1, HS200), SD (sdhc2, cd-gpio PM8994 GPIO8), BLSP I2C/UART/SPI, touch (RMI4), gpio-keys, hall sensor, RPM regulators, QCA6174 Bluetooth, Lattice FPGA manager node, UEFI-compatible reserved-memory.

## Missing subsystems
| Subsystem | Source from stock (DSDT/UEFI) | Mainline cityman | Downstream/other reference | Firmware |
|---|---|---|---|---|
| **Display** (MDSS/MDP + 2xDSI + panel) | MDP 0xFD900000 (len 0xCC000), MMSS_MISC 0xFD828000, vsync INTID 0x73 (SPI 83); reset GPIO78, TE GPIO10; UEFI panel XML: Sergej 1440x2560 cmd-mode, 4 lanes/DSI, bit clk 829.875 MHz, auto-refresh, TE check | **none** (efifb only) | downstream_dts/mmo/panels/dsi-panel-sergej-wqhd-dualdsi-cmd.dtsi, common/mdss.dtsi; LAB/IBB "oled" mode, vddio = pm8994_s4 | not needed |
| **GPU** (Adreno 430) | GFX 0xFDB00000 (0x10000), shader mem 0xFDB20000, INTID 0x41 (SPI 33) | **none** | freedreno a4xx; downstream qcom/msm8994*.dtsi was not brought into this folder (separate path in the repo) | a4xx PM4/PFP microcode (linux-firmware) |
| **USB3/xHCI** | USB0 0xF9200000; QcXhciFilter, qcusbfnss8994 | `usb3@f92f8800` exists in the SoC dtsi, **not enabled** in cityman | UsbTypeC DXE + FPGA | FPGA bitstream: fpga/ice5lp2k_uc120_bitstream.bin (from inside UEFI) |
| **USB HS (ChipIdea)** | UFN1 0xF9A55000 (INTID 0xA6, 0xAC) | **none** | usbfnchipidea.sys | - |
| **Type-C FPGA (iCE5LP2K) + FUSB300** | UEFI UsbTypeCDxe: VREG19 is turned on, FUSB300 over I2C; "calibration file" check | mainline has a `lattice,ice40-fpga-mgr` node | - | bitstream found (see fpga/) |
| **Wi-Fi / BT** | PCI0 0xF8A00000 (len 0x500000), RP1; BT: UART + qcbtuart | **no** PCIe node; BT node exists | README (Lumia950XLPkg): PCIe RP1 initialised by firmware, `pci-host-ecam-generic` suggested | Drivers/bdwlan30.bin, qwlan30.bin, rampatch_tlv_3.x.tlv, nvm_tlv_3.x.bin |
| **Modem** | RFS0 (rmtfs), AMSS, QMUX, IPA; MPSS image | **none** | rmtfs + qrtr | System32/qcdsp28994.mbn (+qcvss8994.mbn, qccpe8994.mbn) |
| **Audio** | ADSP + SLIMbus (SLM1 0xFE12F000, SLM2 0xFE1AF000), codec I2C2 0x40 | **none** | downstream common/audio.dtsi | System32/qcadsp8994.mbn |
| **Camera** | CAMP/CAMS/CAMF/CAMT, VFE0, JPGE (0xFDA0xxxx) | **none** | no camera dtsi in downstream mmo/common (look on the qcom/ side) | - |
| **Battery/charging** | PMIC FG (UEFI PmicDxe), PMBM/PMBT | partial (PMIC exists) | downstream common/charger.dtsi; battery/microsoft_bvt4d_3340mah.dtsi (the 950 XL battery is 3340 mAh; the bvt4d match is inferred from the file name) and bvt5e_3000mah | provision: plat/PROVISION/BATTERY.PROVISION |
| **Haptics / LED / sensors / NFC** | HWN0/1 (qchwnhaptics, qchwnled), SEN1, I2C peripherals | **none** | downstream common/{haptics,sensors,nfc,silabs}.dtsi | - |

## Panel (Sergej) power-on sequence (downstream = UEFI, DCS)
11 (sleep out, 5 ms) → F0 5A 5A (level-2 unlock) → F2 63 → BD 11 01 02 16 02 16 → F0 A5 A5 (lock) → 53 20 → 51 80 → 35 00 (TE on) → 29 (display on).
Power-off: 28, 10. The Duke variant waits 120 ms after 11 and has different BD/FF sequences. Files: panel/panels.json, panel/panel_config_raw.xml.

## Proposed order (the real port work)
1. Mainline DT patch for cityman: enable usb3 + FPGA loading (bitstream from UEFI) + MDSS/DSI/Sergej panel.
2. PCIe (Wi-Fi) and GPU nodes: adapt from the working equivalents in the Android4Lumia950/linux fork for talkman.
3. Firmware extractor script: at install time pull the firmware out of the phone's own UEFI/MainOS partition (not distributed in the repo).
4. ztOS device layer: `uefi-esp` flash method for ztconflo + ztOSIAL manifest.

## Open questions
- The FPGA bitstream end was estimated assuming zero padding; the "calibration file" is a separate source whose location is unknown.
- Which panel (Sergej/Duke) is used may depend on UEFI's DCS ID read; the cityman downstream dtsi contains only Sergej.
- How ready the mainline MDP5/DSI 20nm PHY is for msm8994 was not verified.

## Status update (2026-11-08, measured on the device)
Details: `BRINGUP_LOG.md`.
- **USB3/DWC3 + gadget**: works (TCSR routing + LK QSCRATCH values), Windows ACM + RNDIS, SSH.
- **SD**: works, 3.3 V high speed with `no-1-8-v`. **eMMC** is visible, untouched.
- **Display**: MDP5 v1.9, DSI, panel `sergej` binds; no light: the pclk/byte RCGs are programmed before the DSI PHY PLL runs, `pp done time out`.
- **GPU**: adreno binds; `a420_pm4.fw` missing, `oxili_gfx3d_clk` stuck (iommu resume, GX power domain).
- **Touch** (S3708AR), keys, hall, barometer, magnetometer, SMBCHG/FG: probe.
- **Camera**: the CCS driver does not work with the sensors (dummy supply, CCI timeout); most rows that say "not in mainline" are now in the DT but do not work on the hardware.
- **Bluetooth**: ROME/QCA setup timeout. **Audio**: no ADSP firmware, no card. **NFC**: with the back cover off, polarity detection blocks for 33 s; disabled in the DT.

## Status update (2026-11-10)
- **Display works:** msm DRM + dual DSI + Sergej, 1440x2560, console on the panel; 4/4 clean boots. The root cause of the boot reset is the TZ `restore_sec_cfg` called on the MDP SMMU while UEFI is still scanning; details in `MDP_SMMU_RESET.md`.
- The GPU, camera (CAMSS/VFE), Venus SMMUs and OCMEM are disabled in the DT for now (same SMMU-class problem).
- The signed images needed for ADSP and Venus are identical to the ones in the original FFU (hashes of the LineageOS cityman port); Wi-Fi needs the Android QCA6174 firmware and the PCIe1 BCR reset. Sources: `SOURCES.md`.
- ztOS root boots from the SD's second partition (software rendering, llvmpipe); the A57 cluster and CCI are raised at boot; TSENS thermal zones work (`BRINGUP_LOG.md` 25-31).
