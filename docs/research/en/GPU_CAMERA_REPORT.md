# GPU (Adreno 430) and camera: source consolidation report (2026-11-07)

Türkçe sürüm: [`../GPU_CAMERA_REPORT.md`](../GPU_CAMERA_REPORT.md)

Sources: mainline torvalds/linux (master), Android4Lumia950/linux (master, camss, mdss, iommu, msm8994-6.x branches), Android4Lumia950/android_kernel_mmo_msm8994 (downstream, msm8994-v2.1.dtsi, gpu/camera dtsi), this FFU (DSDT, qccam*.sys, .dcc, SOFTWARE hive).
"VERIFIED" = seen in source code/file. "INFERRED" = conclusion from evidence, not tested on hardware.

## 1. GPU: how complete is the Linux Adreno 4xx driver?

### Mainline (torvalds/linux master)
- VERIFIED: drm/msm a4xx_gpu.c has code for A405/A420/A430. A430 is registered with a single chip id: 0x04030002. Firmware: a420_pm4.fw / a420_pfp.fw (in linux-firmware, listed in WHENCE).
- VERIFIED: the downstream msm8994-v2.1.dtsi that the Lumia uses sets the chip id to 0x04030002 (0x04030000 in the base msm8994-gpu.dtsi). So the 950 XL (v2.1) matches the mainline registration. GPU up to 630 MHz.
- VERIFIED: mmcc-msm8994 has oxili_gfx3d_clk, oxili_rbbmtimer_clk, oxilicx_ahb_clk and OXILI_CX/GX GDSC. The msm8x94 config (rev 9) for MDP5 and the DSI 6G host + msm8994 regulator table exist. The DSI 20nm PHY code exists but needs CONFIG_DRM_MSM_DSI_20NM_PHY.
- MISSING: mainline msm8994.dtsi has no GPU/MDSS node.
- On the freedreno mailing list (December 2015) A430 support was proposed as an "UNTESTED" patch; I did not verify the later test/maintenance history and have no evidence for the health of A430 in mainline.

### Android4Lumia950/linux (master, last commit 2026-09-10) -> this is where the work is
GPU+MDSS are enabled in the DT for cityman ("Enable GPU and MDSS", "Add GPU node", "Add MDSS nodes and its DSIs", "Add iommus", "Add interconnects"). Two driver commits exist and both are explicitly marked temporary/dirty:
1. e8d7ff26a "A4xx: Support A430 for msm8994": chip id 0x04030000 added; RBBM_POWER_CNTL_IP is written before every submit and SP/TP power-on is waited for; the a4xx register dump is shortened for A430 because TZ gives a SEA (error); the OCMEM alloc is moved to after GX is turned on.
2. 6b36f290f "[DIRTY] Add A430 hacks for proper init": a special pre-power-on helper with soc/qcom/msm8994-oxili.h; the first GEM address is 32 MiB instead of 16 MiB (at 16 MiB memptrs gives a permission error); no put_autosuspend in runtime PM (the GPU stays on, otherwise the SoC gives a SEA). Also: "rpmpd: Modify s2b corner for GPU".
- INFERRED: the GPU passes the first hw_init and ring fetch retires (per the commit messages) but (a) power management does not work (the GPU never sleeps, battery), (b) the basic IOMMU/TZ problem (SCM(18) -EINVAL while GX is off) was not solved properly, only bypassed.
- NOT VERIFIED: a real render result with Mesa (kmscube/glmark); the GPU hang rate; a working userspace test after these commits.
- Last note of the msm8994-6.x branch (December 2024): "GPU shows in dmesg, MDSS hangs the kernel". The progress on master is later than this (August-September 2026).

### Conclusion and the "flawless port" path
Your assessment that the GPU "half works" is right: it works but without power management, with hacks. Plan:
1. Take the working base (Android4Lumia950 master) and make it upstream-worthy: bring the hacks down to the root cause one by one.
2. Root-cause source: downstream kgsl 3.10 (power-on order: rail -> clock -> SMMU/TBU, OCMEM/GMEM, secure iommu scm) and Windows qcdxkm8994.sys. We have both in the FFU.
3. Test set: kmscube, glmark2-es2, ztMETRO; hang/idle/suspend loop 1000x.
4. Firmware: a420_pm4/pfp from linux-firmware; downloaded at install time / packaged on the device (redistribution licence to be checked in WHENCE).

## 2. Camera

### Hardware (from the FFU, VERIFIED)
- 3 cameras: rear (RFC, with OIS, ~20 MP, DNG available), front (FFC), "tertiary" = the iris scanner's IR camera (tertiary flash = IR illumination). Windows drivers: qccamrearsensor_primarySMIApp, qccamfrontsensor_secondarySMIApp, qccamsensor_tertiarySMIApp, qccamflash / qccamflashtertiary.
- The sensors are driven through a "SMIA++" (MIPI SMIA/CCS family) wrapper (SMIApp); the drivers configure the sensor from a .dcc (CDCC) file matching the camera ID string read from the NVM. 21 .dcc files: 0A2140xx (Ducati ES1.0, ~1.4 MB), 0AEACAxx (Cooper MP2.0 / Carrera CS5.5), 0C086101 (Jopo/Foxconn), 104541xx (Karma/Mitsumi, BU24210 family, seems to belong to another model family). Details: camera/DCC_INVENTORY.md.
- Registry (camera/REGISTRY_OEM_MMO_Camera.txt): rear I2C 1000 kHz, FlipMirror, max ISO 3200, OIS present; front camera FlipMirror=1; tertiary cropWidthLeft=300.
- SoC block addresses (DSDT == downstream DT, VERIFIED): CCI 0xFDA0C000 (IRQ SPI50), CSID0..3 0xFDA08000/08400/08800/08C00 (SPI51..54), CSIPHY0..2 0xFDA0AC00/0B000/0B400 (SPI78..80), ISPIF 0xFDA0A000 (SPI55), VFE0 0xFDA10000 / VFE1 0xFDA14000 (SPI57/58, "vfe46"), CPP 0xFDA04000, JPEG 0xFDA1C000/20000/24000, IRQ router 0xFDA00000.
- DSDT CAMP GPIO list: 92, 91, 102, 106, 104, 105, 12, 8, 21 (+ one PMIC/extra-controller pin 0x508). Reset/standby pairs: (92,91) rear, (104,105) front, (102,106) iris. This mapping is consistent with the downstream MTP reference and the DT in the camss branch.

### Android4Lumia950/linux `camss` branch (VERIFIED)
- CAMSS_8x94 added: csiphy/csid/ispif/vfe resource tables (csid_ops_4_1, csiphy_ops_2ph_1_0), cci, camss and uc sensor nodes ("nokia,smia") in the cityman DT: Ducati front (@0x20, cci_i2c0, reset 104, regulator l17a 2.7 V), Siiri iris (@0x6c, Toshiba T4KE1, reset 102), Hill rear (@0x20, cci_i2c1, Sony IMX230, reset 92, regulator l29a). Last commit message: "CAMSS and CCI are up(?) ccs not yet."
- This naming matches the FFU: "Found a Siiri sensor" in the driver, T4KE in qcadcm8994.sys, Ducati/Carrera in the DCCs. IMX230 does not appear directly in the FFU (INFERRED, based on the working DT note).

### What is missing = the CCS/sensor layer
- Mainline has a MIPI CCS driver (drivers/media/i2c/ccs); but phone sensors (IMX230 in particular: PDAF, OIS) may not be fully CCS compliant. "ccs not yet" says it does not work yet.
- Windows side: the mode tables and vendor register writes are inside the .dcc ("Register 0x0524 added at the end of OIS VF/VIDEO/STILL MODES"). This suggests that the basic flow (single-mode preview) could work with pure CCS, but the full mode set may need a table layer extracted from the dcc (INFERRED).
- OIS (Rohm BU24210 family and the Windows OIS FW load), the AF actuator and the flash (AS3638 triple LED driver, "no driver available" note) need separate drivers.
- NOT VERIFIED/contradiction: the MCLK assignment in the camss DT (front=MCLK1/gpio14, iris=MCLK2/gpio15) contradicts the Qualcomm MTP reference (gpio15=MCLK2 with 104/105). The real assignment has to be verified live or with the Windows PEP table.
- NOT VERIFIED: link-frequencies 342 MHz; one mode in the .dcc carries a 336 MHz CSI clock (close but not the same).

### Camera to-do (in order)
1. Take the Android4Lumia950 camss + cityman DT into our base kernel; confirm that the sensors are present with a CCI i2c scan (i2cdetect 0x20/0x6c).
2. Read the sensor ID registers (0x0000-0x0006 CCS standard) -> model/rev; compare with the camera ID string in the FFU.
3. Single-mode streaming with the ccs driver; if that fails, the way out: a .dcc mode-table parser (no format document; reverse engineering).
4. Then AF/OIS/flash; the libcamera pipeline (soft ISP) last - Qualcomm's hardware ISP tuning is not to be used.
5. The iris camera is unimportant for ztOS; last.

## 3. Observations (USB / other, short)
- Downstream cityman DT: usb3 maximum-speed = high-speed (no SS on the 950 XL), hsphy init values 0x00D191A4 / 0x00D1D9A4, pcie0 disabled, pcie1 used (Wi-Fi). The UC120 CC PHY and SMBCHG commits are also on master => ready code for the USB gadget.
- Fork master: Wi-Fi/ADSP (remoteproc)/audio (WCD9330, TAS2552) commits exist; their state was not tested.

## 4. Suggested next step
Pull the Android4Lumia950/linux master (+camss) branch completely: (a) produce a numerical diff against mainline, (b) choose it as our base, (c) list the GPU hacks to bring down to the root cause. Another option: keep mainline as the base and rebase the patches one by one (cleaner, slower).

## 5. Update (2026-11-10)
The base was chosen and built (master + camss unique parts, patches 0001-0037). The GPU, its SMMU and OCMEM are disabled in the cityman DT until the power sequence is solved (the MDP SMMU rule applies: no TZ/BFB/context programming before rails and clocks are up and a real domain is attached). Camera/Venus/VFE SMMUs are disabled for the same reason. ztOS currently renders in software (llvmpipe). Timeline: `BRINGUP_LOG.md`.
