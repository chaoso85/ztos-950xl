# Sources / Kaynaklar

What each external source was used for. No proprietary binaries from any of them are in this repository.

## Kernel trees

| Source | Used for |
|---|---|
| [Android4Lumia950/linux](https://github.com/Android4Lumia950/linux) `master` (eb871dc, 2026-09-10) + `camss` | Base tree: cityman DT, MDSS/DSI/Sergej panel, Adreno 430 hacks, UC120, SMBCHG/FG, qcom_iommu MSM8994 support |
| [LineageOS android_kernel_sony_msm8994](https://github.com/LineageOS/android_kernel_sony_msm8994) (cm-14.1, 3.10) | Downstream reference: `msm_iommu_sec.c` (secure page-table pool at subsys_initcall), `msm_iommu-v1.c` (attach order: restore_sec_cfg → BFB → context), `msm8994-iommu.dtsi` (MDP SMMU BFB values, secure-id 1), `mdss_mdp.c` (delay iommu attach until splash handoff), `mdss_mdp_splash_logo.c` |
| Sony-MSM8994-Dev (lineage-21.0, 3.10.108) and sony-msm8994 (lineage-15.1, 3.10.84) | Same downstream code, newer/older; cross-checked the iommu and mdss paths |
| LineageOS android_kernel_oneplus_msm8994 | Same 3.10 base, cross-check |
| msm-3.10 `qpnp-power-on.c` | PMIC PS_HOLD reset type registers (patch 0032) |
| [giannoccarol/android_kernel_mmo_msm8994](https://github.com/giannoccarol/android_kernel_mmo_msm8994) `lineage-23-cityman` | LineageOS 23.2 cityman port: cityman DT (`arch/arm64/boot/dts/mmo/`), panel supplies (vddio = pm8994_s4, PMI8994 LAB/IBB "oled"), Wi-Fi PCIe1 BCR reset, `Documentation/arm64/cityman-{adsp,venus,wifi,ril}.txt`, UC120 driver |
| Android4Lumia950/android_kernel_mmo_msm8994 | Downstream MMO DT (`mmo/common`, `mmo/panels`), panel timings |
| Mainline Linux (torvalds) | drm/msm, qcom_iommu, qcom_scm, clk, interconnect |

## Community notes

| Source | Used for |
|---|---|
| XDA thread: LineageOS 23.2 for Lumia 950 XL (Lorenzogianno) | Working hardware list, firmware hashes: the signed `qcadsp8994.mbn` and `qcvss8994.mbn` match the stock FFU byte for byte; the Android QCA6174 `qwlan30.bin` is needed (the FFU's Windows one does not work) |
| [wiki.nura.eco microsoft-cityman](https://wiki.nura.eco) | Mainline status table (Sep 2026), ath10k notes |
| [Empyreal96 guide](https://empyreal96.github.io/) + [packed-lumia-950xl-kernels](https://github.com/Empyreal96/packed-lumia-950xl-kernels) | efifb-only boot, BT firmware names/hashes (`rampatch_00440302.bin`, `nvm_00440302.bin`), 5.8 config |
| [WOA-Project/linux](https://github.com/WOA-Project/linux) v5.8-rc3-lumia, imbushuo kernel | Early Lumia mainline attempts |
| openSUSE Hack Week (angler/bullhead) | MSM8994 mainline notes, cont_splash handling |
| [Davitekk/ztOS](https://github.com/Davitekk) talkman port | USB HS PHY TCSR routing and LK QSCRATCH values (patches 0014/0015) |
| [Zerone-Laboratories/ztOS](https://github.com/Zerone-Laboratories/ztOS) | Target userland (ztMETRO), ztOSIAL device layer |

## From the phone itself

| Source | Used for |
|---|---|
| Stock Windows 10 Mobile FFU (10586.13169) | ACPI DSDT/SSDT (addresses, IRQs, GPIOs), UEFI panel XML, firmware inventory, camera `.dcc` files and registry |
| UEFI (Lumia950XLPkg) | Boot chain, framebuffer handoff, memory map |
| Stock DSDT (`FUSE` operation region) | CPU speed bin: PTE at 0xFC4BC0B0, `PTE & 7` (this chip: bin 0 = 2.0 GHz part) |
| `QCpep8994.sys` (Windows PEP driver in the FFU) | CPU/CCI frequency and voltage tables (A53 to 1555.2 MHz, A57 to 1958.4 MHz, ceiling 1225 mV) |
| Downstream `clock-cpu-8994.c`, `msm8974-tsens.c`, `msm8992-regulator.dtsi` (Android4Lumia950 MMO kernel) | CPU PLL/mux layout and safe source (AUX/GPLL0), TSENS sensor map and calibration layout, rail ceilings |
