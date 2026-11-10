# Research notes / Araştırma notları

Every Turkish page has an English version under [`en/`](en/); the English-only pages (`MDP_SMMU_RESET.md`, `SOURCES.md`) are not duplicated. Her Türkçe sayfanın İngilizce sürümü [`en/`](en/) altında; yalnızca İngilizce sayfalar (`MDP_SMMU_RESET.md`, `SOURCES.md`) çoğaltılmadı.

| File | TR | EN | Content |
|---|---|---|---|
| BRINGUP_LOG.md | [tr](BRINGUP_LOG.md) | [en](en/BRINGUP_LOG.md) | On-device bring-up chronology (sections 1-31): USB gadget, SD, boot-log audits, SMMU resets, display, ztOS root on SD, CPU/CCI clocks, speed bin, TSENS |
| PORTING_GAPS.md | [tr](PORTING_GAPS.md) | [en](en/PORTING_GAPS.md) | Cross-source gap report: what exists in mainline vs what the phone needs |
| GPU_CAMERA_REPORT.md | [tr](GPU_CAMERA_REPORT.md) | [en](en/GPU_CAMERA_REPORT.md) | Adreno 430 state, camera hardware and plan |
| FORK_ANALYSIS.md | [tr](FORK_ANALYSIS.md) | [en](en/FORK_ANALYSIS.md) | Android4Lumia950/linux analysis, GPU workaround root-cause list |
| FINDINGS_feasibility.md | [tr](FINDINGS_feasibility.md) | [en](en/FINDINGS_feasibility.md) | Initial feasibility notes and the status of each risk |
| DEVICES.md | [tr](DEVICES.md) | [en](en/DEVICES.md) | ACPI device map: addresses, IRQs, GPIOs, I2C (from the stock DSDT) |
| DCC_INVENTORY.md | [tr](DCC_INVENTORY.md) | [en](en/DCC_INVENTORY.md) | Camera tuning file inventory |
| REGISTRY_OEM_MMO_Camera.txt | – | – | Camera registry parameters (language-neutral) |
| MDP_SMMU_RESET.md | – | [en](MDP_SMMU_RESET.md) | Root cause and fix of the boot-time SoC reset (TZ restore_sec_cfg on the MDP SMMU during UEFI scanout) and the runtime DT-node bisection method |
| SOURCES.md | – | [en](SOURCES.md) | External sources (downstream kernels, LineageOS cityman port, community notes) and what each was used for |

Proprietary binaries are intentionally not included. / Tescilli ikili dosyalar bilerek eklenmedi.
