# Android4Lumia950/linux umbrella analysis (2026-11-07)

Türkçe sürüm: [`../FORK_ANALYSIS.md`](../FORK_ANALYSIS.md)

Source: github.com/Android4Lumia950/linux, branches: master (2026-09-10), camss (2026-08-29), mdss, iommu, msm8994-6.x. Author: sonic011gamer (+ the qcom_iommu series taken from Dmitry Baryshkov).
Custom commit count (master, after 2026-08-20): 43; files touched: 71. Full list: ztos-lumia/fork_master_commits.txt (cloud workspace).
IMPORTANT: the cityman display panel (Sergej, panel-microsoft-sergej.c) and CAMSS 8x94 are in the `camss` branch; master has the talkman panel (AMS567DJ06). The full base = master + camss merged (not merged yet at the time of writing; done later, see the README).

## Subsystem table
| Subsystem | What the fork has | Status label (from the commit message) |
|---|---|---|
| GPU (A430) | a4xx_catalog 0x04030000/02, a4xx_gpu SP/TP power-on, OCMEM delayed alloc, msm8994-oxili pre-power-on helper, GEM 32 MiB start, no autosuspend, rpmpd s2b corner, ubwc no_config, ocmem 8994 quirk, mmcc GPU workaround | [DIRTY], [CLEAN UP LATER], [FIXUP LATER] |
| Display (MDP5/DSI) | 20nm DSI PHY msm8994/92 support, dsi_cfg msm8992, mdp5 dual LM/ping-pong + cmd encoder + ctl changes, panel-microsoft-sergej (camss branch), Samsung AMS567DJ06 (talkman) | [NEED REWRITE/REVIEW?], [NEEDS CLEAN IMPL], WIP |
| IOMMU/interconnect | qcom_iommu 8974/8994 (Baryshkov series + MDP/GPU iommu), msm8994 interconnect driver + binding | [decommonize later] |
| Camera | camss 8x94 (csiphy 2ph_1_0, csid 4_1, ispif, vfe), cityman DT: cci + 3 "nokia,smia" sensors, pinctrl | "ccs not yet" |
| USB-C / charging | Lattice UC120 CC PHY, PMI8994/96 SMBCHG, PMI8994 fuel gauge | [SPLIT LATER], [NEEDS REVIEW] |
| Audio / modem | WCD9330 codec, TAS2552, q6 ADSP remoteproc 8994, slimbus, SoC machine driver, SCM PIL shared heap | [CLEAN UP LATER] |
| Other | talkman camera key fast reboot [DNM], mmo_defconfig [DNM], memory map for LK [REVERT LATER] | - |

## Verification: the Sergej panel init sequence (3 independent sources agree)
fork panel-microsoft-sergej.c == downstream mmo dtsi == UEFI display DXE (my FFU extraction):
11 (sleep out, 5 ms) -> F0 5A 5A -> F2 63 -> BD 11 01 02 16 02 16 -> F0 A5 A5 (23 ms) -> 53 20 -> 51 80 -> 35 00 -> 29. Power-off: 28 (33 ms) -> 10 (120 ms).
Note: UEFI's Sergej/Duke selection (ID read) is still unverified; the Sergej assumption for the 950 XL is based on the downstream cityman dts.

## GPU hacks -> root-cause research list (for the "flawless port")
1. GEM permission error at 16 MiB (memptrs): a likely cause is an SMMU context-bank / secure-aperture clash or a reserved VA region of the A430. See downstream kgsl iommu "secure" and the global pagetable range, qcom_iommu BFP/prefetch settings.
2. SoC SEA on autosuspend: SCM(18) (restore_sec_cfg) -EINVAL while GX is off. The right fix: put the GX/CX GDSC + clock order into the runtime-PM resume callback and remove the "always on" hack. See the downstream kgsl_pwrctrl order (rail -> clk), mmcc GDSC dependencies (oxili_gx parent = oxili_cx).
3. SP/TP power-on wait: related to the early-revision problem of the A430 patch (RBBM_CLOCK_CTL); since the device is v2.1 (0x04030002) this path may be unnecessary. Test: hack-free hw_init + clock gating setting.
4. TZ SEA in the a4xx register dump: removing the XPU/VMIDMT ranges from the dump list is the permanent right fix (A430 only).
5. OCMEM: GMEM allocation after GX; the ocmem 8994 quirk must be cleaned up.
6. Mesa side: real rendering with freedreno a4xx is unverified -> first test kmscube/glmark2-es2.

## Suggested working method
A) Fast path: take fork master+camss as the base, build, try on the device (the USB gadget is needed for rescue: UC120 + usb3 high-speed).
B) Clean path: rebase the patches onto mainline as a series (clean the labelled ones first). Starting with A and moving to B makes sense.
Next job: build the kernel (aarch64 cross) and produce the fork master+camss merge and report the obvious conflicts. (Done: patches 0001-0037, see `BRINGUP_LOG.md`.)
