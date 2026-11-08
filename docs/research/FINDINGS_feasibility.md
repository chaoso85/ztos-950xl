# ztOS -> Lumia 950 XL (cityman): fizibilite notları (2026-11-07)

## ztOS mimarisi (repolar klonlandı: ztos-ial, ztconflo, ztos-devices)
- GSI (ztMETRO + apps + SDK, Arch Linux ARM aarch64) + cihaz katmanı (/vendor/ztos/manifest.toml)
- ztOSIAL: D-Bus arayüzleri (Display1, Modem1, Power1, Sensors1, Haptics1, Lights1 ...).
  Manifest yoksa generic backend'ler kullanılır; yetmezse Python backend (örn. starlte.py) yazılır.
- ztconflo: codename -> boot.img + rootfs.img. Sadece fastboot/heimdall/twrp flash yöntemleri var;
  Lumia için "uefi-esp" gibi yeni bir flash_method eklenmesi gerekir. Eligibility filtresi mainline + Mesa GPU ister.
- Registry'de samsung-j5 (msm8916, pmOS kernel, mainline 6.12, freedreno) = Qualcomm için en yakın örnek:
  rmtfs, msm-firmware-loader, qrtr, UCM profilleri hazır -> Lumia'ya şablon olabilir.

## Mainline cityman (arch/arm64/boot/dts/qcom/msm8994-msft-lumia-octagon{,-cityman}.dts*)
VAR/etkin: sdhc1 (eMMC, HS200), sdhc2 (SD kart, cd-gpio), touch (Synaptics RMI4, i2c 0x4b), yan kapasitif tuşlar,
  gpio-keys, hall sensör, UART'lar, PMIC/RPM regülatörleri, QCA6174 Bluetooth, Lattice iCE40 FPGA (USB-C PD PHY) manager,
  reserved-memory UEFI düzenine göre kopya, chosen: bootargs "earlycon=efifb console=efifb acpi=no".
YOK/etkin değil: USB (usb3 SoC dtsi'de var, cityman'da etkin değil), MDSS/DSI panel, GPU, PCIe (Wi-Fi), modem, kamera, ses.

## WOA UEFI (Lumia950XLPkg)
- Windows ARM64 ve Linux boot eder; ACPI sadece Windows için -> Linux'ta DT + acpi=no.
- Ekran = UEFI'nin bıraktığı framebuffer (efifb); MDP tam implemente değil.
- PCIe RP1 firmware tarafından init edilmiş: pci-host-ecam-generic ile kullanım öneriliyor (ath10k Wi-Fi için ayrıca IRQ gerekli).
- README SD karttan boot'u anlatmıyor.

## Topluluk deneyimi (Matt Miller, Debian on Lumia 950 XL)
- Debian 12, WOA UEFI üzerinden GRUB (\EFI\Microsoft\Boot\bootmgfw.efi olarak kopyalanmalı, fallback yolu çalışmıyor).
- Eski 5.7 kernel (empyreal96) çalışıyor: efifb ekran, touch, eMMC, Bluetooth. USB/Wi-Fi/GPU/modem/batarya/ses yok.
- Mainline 6.16 + pmOS config + cityman DT: framebuffer'a kadar boot, sonra RPM regülatör init'inde takılma.
- Güç: PSCI eksik, kapatma/yeniden başlatma güvenilmez.
- Ağ yolu: Bluetooth PAN (USB yok).

## Riskler / açık sorular
1. USB gadget (SSH/rescue) yok -> ilk hedef: usb3 + Lattice PD PHY çözmek (en büyük bloklayıcı).
2. Mainline RPM regülatör hang'i (6.16). Önce 5.7 ile çalışan tabana dayanıp yukarı taşımak daha güvenli.
3. GPU/MDSS yok: ztMETRO'nun GLES2 renderer'ı için simpledrm + llvmpipe denenmeli (performans/uyumluluk doğrulanmadı).
4. SD karttan boot UEFI'de doğrulanmadı; eMMC üzerinden ESP/GRUB yöntemi kanıtlı. SD'den boot için UEFI'nin SD'den
   okuyabildiği test edilmeli (Lumia950XLPkg SD desteği var ama "bazı kartlar desteklenmiyor").
5. EFS/MODEM/SBL/TZ/RPM bölümlerine dokunulmayacak; eMMC tam yedek zorunlu.

## Güncelleme (2026-11-08)
- Risk 1 (USB gadget): **çözüldü**, Windows ACM+RNDIS ve SSH çalışıyor.
- Risk 4 (SD'den açılış): **çözüldü**: stock UEFI -> rEFInd -> SD üzerindeki GRUB (`\EFI\BOOT\BOOTAA64.EFI`) -> Linux.
- Risk 2 (RPM takılması): bu çekirdekte takılma yok; RPM regülatörleri geç ama açılıyor (~8 sn).
- Risk 3 (GPU/MDSS): MDP5/DSI/GPU bağlanıyor ama ekran henüz yanmıyor (bkz. `BRINGUP_LOG.md`).
