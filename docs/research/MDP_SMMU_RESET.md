# MSM8994 MDP SMMU: SoC reset at boot (root cause and fix)

Device: Microsoft Lumia 950 XL (cityman, MSM8994), Linux 7.3-rc2 (Android4Lumia950/linux master base), booted by the
stock Windows-on-ARM UEFI with its framebuffer still on screen. Full chronology (Turkish): `BRINGUP_LOG.md` sections 12–23.

## Symptom

With the display (MDSS/MDP/DSI) and the MDP SMMU enabled in the device tree, most boots ended in a silent SoC reset
somewhere in the deferred-probe burst (about 24–25 s with `initcall_debug`, before init without it). There was no oops or
panic. The PMIC reported `poff 02` (PS_HOLD), meaning the secure world reset the SoC. When a boot survived, the panel worked.
The SAFE DT, with MDSS and all SMMUs off, never reset.

## Why it took long to find

- The fbcon console lags real time by several seconds, then flushes its backlog. Counting video frames showed kernel time going
  from 0.4 s to 24.9 s in about 4 s of real time. **The last line visible on the panel is not where the reset happened.**
  Several wrong suspects (OCMEM, the TZ secure page-table pool and where it lives, when it is initialised) came from reading
  those last lines.
- ramoops/pstore did not survive the reset. A PMIC warm reset (`qcom_pon.ps_hold_warm=1`) did not help either: the UEFI
  does not preserve the region.
- The reset happens before init, so the SD-card logger in the rescue initramfs never captured it.

## Method that found it

1. Boot the SAFE DT, which always survives. Its USB gadget and SSH come up at ~3 s.
2. Stream `/dev/kmsg` over SSH to the PC.
3. Enable the FULL-only DT nodes **at runtime, one by one**, with a tiny module (`rescue/tools/ztdt.c`). The module sets
   `status = "okay"` through an `of_changeset`, and the OF reconfig notifiers then create the platform/I2C device.
4. If the phone resets, the default GRUB entry boots SAFE again and the loop continues without anyone at the phone.

Fuel gauge, the proximity sensor, and CCI were fine. Enabling `iommu@fd9cc000` (the MDP SMMU) reset the phone every time.
Temporary step markers in `qcom_iommu` (each followed by a 400 ms pause, so the streamed log keeps up) showed the exact step:

```
iommu sec: pgtable size: 151552
mdp-step: PTBL_INIT ret 0
mdp-step: probe: populating ctx banks
mdp-step: resume: clocks on
mdp-step: resume: calling restore_sec_cfg(1, 0)
<reset>
```

## Root cause

The context-bank probe runtime-resumed the parent SMMU. The resume calls the TZ SCM `restore_sec_cfg(sec_id = 1, 0)`
for the MDP SMMU while the UEFI framebuffer is **still being scanned out** through it from physical addresses. TZ
reprograms the SMMU's secure configuration under a live display fetch, and the SoC resets.

Downstream 3.10 avoids exactly this. `drivers/video/msm/mdss/mdss_mdp.c` says: *"delay iommu attach until continous splash
screen has finished handoff, as it may still be working with phys addr"*. In `msm_iommu-v1.c`, `restore_sec_cfg` and the
BFB setup happen at attach time. An earlier patch here (0026) only deferred the *domain* attach, which gave an identity
default domain. The driver's own resume still issued the TZ call during probe.

## Fix (kernel patch 0034)

For the MSM8994 MDP SMMU (`qcom,msm8994-mdp-iommu`):

- `hw_deferred` is set at probe.
- Runtime resume only enables clocks while it is set.
- The context-bank probe no longer resumes the parent. `ioremap` does not touch the hardware.
- The first attach of a translating domain does `restore_sec_cfg` + BFB, clears the flag, and then programs the context.
  That attach comes from msm DRM, after `mdp5_kms_init()` has stopped the UEFI scanout.

Related patches:

- 0031: the TZ secure page-table pool (`IOMMU_SECURE_PTBL_SIZE/INIT`) is set up once from `qcom_scm` probe, as
  `msm_iommu_sec_init()` does at subsys_initcall.
- 0026: identity default domain for the MDP device until DRM attaches.

Result: 4/4 clean FULL DT boots. msm DRM drives the Sergej panel at 1440x2560 with a console on it, and init starts at ~22.5 s.

## Probably the same class of bug elsewhere

The GPU, Venus and camera (VFE) SMMUs reset or hung the SoC when they attached with their blocks unpowered (sections
9–10). They are disabled for now. When they come back, apply the same rule as downstream: no TZ/BFB/context programming
until the block's rails and clocks are up and a real domain is attached.

## Tools

- `rescue/tools/ztdt.c`: enable a DT node at runtime (`insmod ztdt.ko node=/soc@0/iommu@fd9cc000`).
- `rescue/tools/ztsec.c`: run the TZ secure page-table SCM calls by hand from a module and log each step.
