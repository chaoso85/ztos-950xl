# ztOS -> Lumia 950 XL (cityman): feasibility notes (2026-11-07)

Türkçe sürüm: [`../FINDINGS_feasibility.md`](../FINDINGS_feasibility.md)

## ztOS architecture (repos cloned: ztos-ial, ztconflo, ztos-devices)
- GSI (ztMETRO + apps + SDK, Arch Linux ARM aarch64) + device layer (/vendor/ztos/manifest.toml)
- ztOSIAL: D-Bus interfaces (Display1, Modem1, Power1, Sensors1, Haptics1, Lights1 ...). With no manifest the generic backends are used; if they are not enough, a Python backend (e.g. starlte.py) is written.
- ztconflo: codename -> boot.img + rootfs.img. Only fastboot/heimdall/twrp flash methods exist; a new flash_method such as "uefi-esp" is needed for the Lumia. The eligibility filter wants mainline + a Mesa GPU.
- In the registry samsung-j5 (msm8916, pmOS kernel, mainline 6.12, freedreno) = the closest Qualcomm example: rmtfs, msm-firmware-loader, qrtr, UCM profiles are ready -> could be a template for the Lumia.

## Mainline cityman (arch/arm64/boot/dts/qcom/msm8994-msft-lumia-octagon{,-cityman}.dts*)
PRESENT/enabled: sdhc1 (eMMC, HS200), sdhc2 (SD card, cd-gpio), touch (Synaptics RMI4, i2c 0x4b), side capacitive keys, gpio-keys, hall sensor, UARTs, PMIC/RPM regulators, QCA6174 Bluetooth, Lattice iCE40 FPGA (USB-C PD PHY) manager, reserved-memory copied per the UEFI layout, chosen: bootargs "earlycon=efifb console=efifb acpi=no".
MISSING/not enabled: USB (usb3 is in the SoC dtsi, not enabled in cityman), MDSS/DSI panel, GPU, PCIe (Wi-Fi), modem, camera, audio.

## WOA UEFI (Lumia950XLPkg)
- Boots Windows ARM64 and Linux; ACPI is for Windows only -> DT + acpi=no on Linux.
- Display = the framebuffer UEFI leaves behind (efifb); MDP is not fully implemented.
- PCIe RP1 is initialised by firmware: use with pci-host-ecam-generic is suggested (ath10k Wi-Fi additionally needs an IRQ).
- The README does not describe booting from SD.

## Community experience (Matt Miller, Debian on Lumia 950 XL)
- Debian 12, GRUB through the WOA UEFI (must be copied as \EFI\Microsoft\Boot\bootmgfw.efi, the fallback path does not work).
- The old 5.7 kernel (empyreal96) works: efifb display, touch, eMMC, Bluetooth. No USB/Wi-Fi/GPU/modem/battery/audio.
- Mainline 6.16 + pmOS config + cityman DT: boots up to the framebuffer, then hangs in RPM regulator init.
- Power: PSCI is missing, shutdown/reboot is unreliable.
- Network path: Bluetooth PAN (no USB).

## Risks / open questions
1. No USB gadget (SSH/rescue) -> first goal: solve usb3 + the Lattice PD PHY (the biggest blocker).
2. Mainline RPM regulator hang (6.16). Building on the working 5.7 base and moving it up is safer.
3. No GPU/MDSS: simpledrm + llvmpipe should be tried for ztMETRO's GLES2 renderer (performance/compatibility unverified).
4. Booting from SD is unverified in UEFI; the ESP/GRUB method over eMMC is proven. UEFI's ability to read from SD must be tested (Lumia950XLPkg has SD support but "some cards are not supported").
5. EFS/MODEM/SBL/TZ/RPM partitions will not be touched; a full eMMC backup is mandatory.

## Update (2026-11-08)
- Risk 1 (USB gadget): **solved**, Windows ACM+RNDIS and SSH work.
- Risk 4 (boot from SD): **solved**: stock UEFI -> rEFInd -> GRUB on the SD (`\EFI\BOOT\BOOTAA64.EFI`) -> Linux.
- Risk 2 (RPM hang): no hang in this kernel; the RPM regulators come up late (~8 s).
- Risk 3 (GPU/MDSS): MDP5/DSI/GPU bind but the display is not lit yet (see `BRINGUP_LOG.md`).

## Update (2026-11-10)
- Risk 3 is answered: the display works with msm DRM, and ztMETRO runs on **llvmpipe** (software GLES 3.2) as the ztOS root on the SD card's second partition; it is slow (3-5 fps at the bootloader CPU clocks, 7-9 fps after the A57/CCI boost) until the GPU is enabled.
- Risk 2/PSCI: `safe-reboot` works reliably; shutdown/reboot of the phone itself still goes through PS_HOLD.
