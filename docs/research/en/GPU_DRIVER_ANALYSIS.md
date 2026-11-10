# Adreno 430 power-sequence analysis: Windows 10 Mobile + WOA + other kernels

Turkish original: [`../GPU_DRIVER_ANALYSIS.md`](../GPU_DRIVER_ANALYSIS.md)

Goal: compare the power/clock sequences of three sources to fix the GPU SMMU attach that resets/hangs the SoC. In order: (1) the original Windows 10 Mobile (FFU) drivers, (2) WOA-Project/Lumia-Drivers, (3) other kernels (downstream MMO msm-3.10, the fork, mainline). No proprietary binaries are in the repo; findings only.

## 1. Windows 10 Mobile (FFU)

- One ACPI device covers the GPU: `GPU0` (QCOM246F), display + Adreno 430 + Venus. Power sequencing is done by the PEP block in the DSDT (`QCpep8994.sys`).
- GPU SMMU = MMU3 (`0xFDB10000`, size 0xB000, IRQs 0x46 0x111 0x112 0x110, 3 context banks, 3 `S2CB` entries). Driver `qcsmmu8994.sys` makes **no SCM/TZ calls**; it programs registers directly.
- The GPU core clock is the RPM resource `/clk/oxili` (gfx3d_clk_src, RPM mem-clk type, id 1); the GX rail is PMIC client `rail_gfx`.
- Power-up: `/clk/oxili` = 19.2 MHz → GX GDSC (VDD_OXILI) → CX GDSC (VDD_OXILICX) → bus vote (APPSS→GFX3D_CFG) → `oxili_gfx3d_clk`, `oxilicx_ahb_clk`, `oxili_rbbmtimer_clk`. Power-down in reverse (clocks off, bus 0, GX then CX off, `/clk/oxili` = 0). Clock resets use assert/deassert (0xA/0xB, 1 µs).
- Microcode versions differ from linux-firmware (pm4 0x4ff062 / pfp 0x4ff085 vs 0x4ff065 / 0x4ff087): a version difference only.

## 2. WOA Lumia-Drivers

- The ARM32 system files are identical to the FFU. The ARM64 version is older, a native AArch64 build, easier to disassemble. Same sequence; nothing new. The "not perfect" note: the GPU only works for the display path, 3D acceleration is incomplete.

## 3. Other kernels

- **Downstream MMO msm-3.10:** `kgsl_pwrctrl`: GX then CX rail, clocks, AXI vote, then A4xx SP/TP power-on (`RBBM_POWER_CNTL_IP = 0x778000`, poll `SP_TP_PWR_ON`). The GPU SMMU (`kgsl_iommu`) also powers GX and CX (`__enable_regulators`, alt-vdd). `gdsc_enable` enables the root clock before releasing SW_COLLAPSE and deasserts clock resets on the non-logic path. v2.1 overlay: chipid `0x04030002`, single 630 MHz level.
- **Our tree / the fork:** `oxili_gx_gdsc` and `oxili_cx_gdsc` exist in mmcc (`0x4024`/`0x4034`), GX has a `vdd-gfx` supply. The fork's mmcc "GPU workaround" powers GX+CX before the SMMU (`msm8994_oxili_pre_gpu_power`) and sets the root clock to 300 MHz before GX (3.10 `initial-pwrlevel 2`). rpmpd votes SLEEP as well as ACTIVE for `gfx_s2b_corner` (3.10 `qcom,set = <3>`). a4xx_gpu.c: A430 SP/TP power-on is in `a4xx_pm_resume`/`a4xx_submit` (the wait loop in resume is unbounded).
- **Mainline:** no A430 support; only via the fork's e8d7ff26a8f.

## 4. Differences from our DT (likely causes of the SMMU hang)

1. `gpu_iommu` is powered by `OXILI_CX_GDSC` only; Windows and downstream power **GX too**. Touching a context bank with GX off → hang/reset.
2. The DT has 2 context banks, both SPI 241; the hardware has 3 (IRQs SPI 240/241/242, the third secure, secure-id 18).
3. Windows makes no TZ call; `restore_sec_cfg(18)` may be downstream-only (unverified). The MDP fix in 0034 defers it until a real domain is attached.
4. Clock order: Windows sets the root clock to 19.2 MHz first, then the GDSCs. Ours uses 300 MHz before GX; a small difference, but following the Windows order is safer.
5. The unbounded wait loop in `a4xx_pm_resume` needs a timeout.

## 5. Proposed hardware plan (not tried yet)

1. DT: add the 3rd bank and IRQs to `gpu_iommu`, `power-domains` = GX + CX; nodes stay `disabled`.
2. With `ztdt.ko`, power only GX+CX at runtime and read the `GDSCR` status bits (over SSH, without touching the SMMU).
3. If that works, attach the GPU SMMU (no TZ); then the `msm` GPU node; `kmscube`/`glmark2-es2`.
4. Before every phone reboot the user is told why.

## 6. Hardware experiments (2026-11-10, rescue, `ztgpupwr.ko` / `ztgfxrail.ko`)

| Experiment | Result |
|---|---|
| Power GX + CX GDSC by hand (the fork's `msm8994_oxili_pre_gpu_power`) | Works: GDSCR `a0282000` (PWR_ON, SW_COLLAPSE=0), phone alive |
| + the three clocks (gfx3d 300 MHz, cx_ahb, rbbmtimer) | CLK_OFF clear in the CBCRs (`0x4028=00006ff1`, `0x403c=20008001`, `0x40b0=1`) |
| + `rpmpd gfx` corner vote (state 3 = svs_soc) | `gfx` domain on, PM8994/pmi8994 S2 in use |
| pmi8994 S2/S3 (VDD_GFX) SPMI dump | Already enabled at boot (`enable 80`, `status1 81`): the rail is not the cause |
| Read GPU SMMU `ID0..2` (`smmu=1`) | **CPU hangs** (not a reset: screen stops at ~183 s, USB dies) |
| Read GPU `0xfdb00000` reg0..3 (`gpu=1`), rail vote included | **CPU hangs** |

Conclusion: every power, rail and clock precondition documented for Windows/downstream is met, yet the first access to both the GPU and the GPU SMMU register spaces still hangs (a slave that never answers). Candidates not yet eliminated: (a) the Windows `APPSS→GFX3D_CFG` configuration-path bus vote (mainline interconnect has no equivalent: `slv_cnoc_mnoc_mmss_cfg`/`slv_mmss_clk_xpu_cfg` exist but no GFX3D_CFG node), (b) XPU/MPU protection (`slv_mnoc_mpu_cfg`; unverified, and an earlier reset was wrongly blamed on XPU), (c) a step `qcdxkm8994.sys` takes before its first register read (TZ/SMMU init or a GPU block reset). Next step is not another live test but offline analysis of the code in `qcdxkm8994.sys` (the WOA ARM64 build) up to the first RBBM access.

### 6a. Further experiments (same day)

- PEP re-read (`gpu_pep.txt`): power-up = `/clk/oxili`=0x4B00 (19.2 MHz) → GX → CX → `BUSARB APPSS_PROC→GFX3D_CFG 0x11E1A300 (=300000000)` → the three clocks. A separate `PSTATE 1`: reset assert (0x0A), 1 µs, deassert (0x0B) on the three clocks.
- The `GFX3D_CFG` vote was sent to the RPM (`bmas` master 0 / `bslv` slave 11, bw 300000000): the RPM answered **"resource does not exist" (-ENXIO)**; this phone's RPM has no bus-arbiter resources (which is why `ignore_enxio` is set everywhere in the driver).
- The RPM `mmssnoc_ahb_clk` (MMSS NoC AHB root) had never been voted; it was (user count 1). **Result: the GPU reg0 read still hung the CPU.**
- In `qcdxkm8994.sys`, `GpuResetValue`/`BusResetValue` are timeout settings, not a hardware reset.
- Remaining candidates: the clock reset pulse (PSTATE 1) and starting the root clock at 19.2 MHz as Windows does, XPU/MPU, another step inside `qcdxkm8994.sys`.
