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

## 7. Review of the working Android sources (giannoccarol cityman, Android4Lumia950 lk2nd, mainline, marlin)

- **Android (cityman, kgsl) uses the same sequence as ours:** `pwrlevel_change` → GX (`gdsc_oxili_gx`, `parent-supply = pmi8994_s2_corner`, an RPM-smd-regulator, `qcom,set=3`) → CX → clocks (`core`, `iface`, `rbbmtimer`) → AXI vote → A430 SP/TP (`RBBM_POWER_CNTL_IP = 0x778000`). kgsl itself makes **no SCM/TZ call** and no configuration-path (GFX3D_CFG) vote (only the data path 26→512). So the Windows-specific `APPSS→GFX3D_CFG` vote is not needed on Android.
- **SMMU (`msm_iommu-v1`) first-attach order:** regulators (CX + alt-GX) → `apply_bus_vote` (26→512) → clocks → SCM `restore_sec_cfg(18)` → BFB → context. Register access comes **after** the TZ call.
- **SMPB2 (pmi8994 S2/S3, VDD_GFX) is RPM-owned on Android** (`rpm-regulator-smpb2`, corner vote). Our DT also models it as an SPMI regulator (`vdd_gfx`, 980 mV). Two owners may collide; the rpmpd corner numbering (downstream `init-voltage-corner = <2>` commented SVS SOC, enum NONE=1…) was not matched or verified.
- **lk2nd (msm8994):** no GPU-specific init; only `scm_xpu_err_fatal_init()` (makes XPU violations fatal) and the MMSS AXI clocks. The Windows UEFI → GRUB path has the same TZ/XPU state, but how an XPU violation presents on this SoC (hang vs reset) is not verified.
- **Mainline v7.3.1 `a4xx_gpu.c`:** 22894 bytes, identical to `master`; no A430 power code. Nothing new beyond the fork's e8d7ff26a8f. The freedreno repo is userspace tooling only (`a4xx.xml.h`), no kernel power sequence. The marlin `adreno.h` only has feature flags (`ADRENO_SPTP_PC` etc.).
- **Conclusion:** if the GPU register read still hangs although we follow Android's sequence, the difference is in platform state or in the SMMU/SCM prerequisite. The next sensible experiment is to follow Android's real path instead of poking from a module: `restore_sec_cfg(18)` with GX+CX+clocks up (like 0034 for MDP, but for the GPU SMMU), and removing the SPMI/RPM double ownership.

## 8. Comparison with Davitekk/ztOS `talkman-port` (Lumia 950, GPU works)

Sources: `Davitekk/ztOS` branch `talkman-port` (`devices/microsoft-talkman`), the `Android4Lumia950/linux` (sonic011gamer) GPU patches (`102627e`, `6b36f290`, `afc1c428`), `lk2nd`, `gus33000/boot-shim`.

- **Davitekk's DT enables the GPU with just `&gpu { status = "okay"; }`**, on the same kernel fork (`eb871dc8`). Nothing is missing in DT; the differences are our own **disables** (`gpu`, `gpu_iommu`, `ocmem`) and our test module.
- **The fork's own comment is the key:** in `qcom_iommu.c`, "*GPU SMMU is TZ-managed (secure-id 18) … TZ owns GR0, do not touch GR0*". The GPU SMMU **global space (GR0) belongs to TrustZone**; only context banks, and only after `restore_sec_cfg(18)`, are usable. My `smmu=1` experiment read `0xfdb10020` (GR0 ID registers): wrong target, and a hang is the expected result. The 2026-11-09 "iommu group 0" reset is explained the same way: an early `gpu_iommu` resume with `pre_gpu_voted()` false falls into `qcom_iommu_reset_ns()` (a GR0 write).
- **Real driver order:** `msm8994_oxili_pre_gpu_power()` (GX→CX) → `pm_runtime_get_sync(gpu)` → SMMU supplier resume → `qcom_scm_restore_sec_cfg(18,0)` → only then GPU registers. My test module **never called `restore_sec_cfg(18)`** and touched the GPU core register (0xfdb00000) before it. That TZ opens the GFX3D XPU partition with this call is unverified, but the order matches Android (section 7).
- **Boot path:** Davitekk: UEFI → BootShim (AArch32; after `ExitBootServices` SMC `0x0200010f` = SCM milestone 32→64, `el1_elr`=LK) → lk2nd → Android boot image. lk2nd deliberately disables `ENABLE_XPU_VIOLATION` ("primary bootloader does it") and does nothing GPU-specific; its MMSS AXI clocks are display-only. Ours is UEFI → GRUB. Whether the TZ state differs between the two paths **cannot be determined from sources**.
- **8992 vs 8994:** Davitekk runs on 8992 (A418 as A430). The fork's 8994 guard ("First hw_init works") says sonic011gamer's first `hw_init` ran on 8994; our real path has never been run yet.
- **Conclusion / plan (no phone access):** (1) no more raw MMIO reads. (2) A `gpu_iommu` patch for the 8994 GPU branch that never touches GR0 (no `reset_ns`) and defers hardware access until after the first real `pre_gpu_power`, like `hw_deferred` for MDP. (3) Enable `gpu`, `gpu_iommu`, `ocmem` in DT and run the real driver path once with the SD logger on. Only when the user gives the connection, with the reboot reason stated first.
