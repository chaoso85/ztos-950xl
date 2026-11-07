# Lumia 950 XL (cityman, RM-1085): mainline Linux için eksik donanım tanımları

Üç bağımsız kaynak çapraz karşılaştırıldı:
1. **Stock W10M FFU** (10586.13169): PLAT\ACPI DSDT/SSDT, UEFI DXE sürücüleri, MainOS sürücü/firmware dosyaları
2. **Mainline Linux** (torvalds/linux master): msm8994.dtsi + msm8994-msft-lumia-octagon{.dtsi,-cityman.dts}
3. **Android4Lumia950/android_kernel_mmo_msm8994**: downstream arch/arm64/boot/dts/mmo/* (bu klasörde downstream_dts/)

## Çapraz doğrulama sonuçları (veri güvenilirliği)
| Kontrol | Sonuç |
|---|---|
| Dokunmatik: DSDT TSC1 = I2C1 0x4B | mainline DT `rmi4-i2c@4b` ile aynı |
| Panel zamanlamaları: UEFI XML (Sergej) vs downstream `dsi-panel-sergej-...dtsi` | HFP 76, HBP 32, HSYNC 16, VFP 4, VBP 2, VSYNC 2, 60 Hz, komut modu: **birebir aynı** |
| Panel GPIO'ları: DSDT DISP (GpioInt 0x0A, GpioIo 0x4E) vs downstream mdss.dtsi (te-gpio 10, reset-gpio 78) | **aynı** (GPIO10 = TE, GPIO78 = reset) |
| Seçilen panel: downstream msm8994-chi.dtsi yalnızca Sergej'i içe aktarıyor | cityman = Sergej (Duke da UEFI'de tanımlı) |

## Mainline cityman'da şu an etkin olan (stock verisiyle uyumlu)
eMMC (sdhc1, HS200), SD (sdhc2, cd-gpio PM8994 GPIO8), BLSP I2C/UART/SPI, dokunmatik (RMI4), gpio-keys, hall sensör,
RPM regülatörleri, QCA6174 Bluetooth, Lattice FPGA manager düğümü, UEFI'ye uygun reserved-memory.

## Eksik alt sistemler
| Alt sistem | Stock'tan kaynak (DSDT/UEFI) | Mainline cityman | Downstream/diğer referans | Firmware |
|---|---|---|---|---|
| **Ekran** (MDSS/MDP + 2xDSI + panel) | MDP 0xFD900000 (len 0xCC000), MMSS_MISC 0xFD828000, vsync INTID 0x73 (SPI 83); reset GPIO78, TE GPIO10; UEFI panel XML: Sergej 1440x2560 cmd-mode, 4 şerit/DSI, bit clk 829.875 MHz, auto-refresh, TE check | **yok** (yalnızca efifb) | downstream_dts/mmo/panels/dsi-panel-sergej-wqhd-dualdsi-cmd.dtsi, common/mdss.dtsi; LAB/IBB "oled" modu, vddio = pm8994_s4 | gerekmiyor |
| **GPU** (Adreno 430) | GFX 0xFDB00000 (0x10000), shader mem 0xFDB20000, INTID 0x41 (SPI 33) | **yok** | freedreno a4xx; downstream qcom/msm8994*.dtsi bu klasöre alınmadı (depoda ayrı yolda) | a4xx PM4/PFP microcode (linux-firmware) |
| **USB3/xHCI** | USB0 0xF9200000; QcXhciFilter, qcusbfnss8994 | SoC dtsi'de `usb3@f92f8800` var, cityman'da **etkin değil** | UsbTypeC DXE + FPGA | FPGA bitstream: fpga/ice5lp2k_uc120_bitstream.bin (UEFI içinden) |
| **USB HS (ChipIdea)** | UFN1 0xF9A55000 (INTID 0xA6, 0xAC) | **yok** | usbfnchipidea.sys | - |
| **Type-C FPGA (iCE5LP2K) + FUSB300** | UEFI UsbTypeCDxe: VREG19 açılıyor, I2C ile FUSB300; "calibration file" kontrolü | mainline'de `lattice,ice40-fpga-mgr` düğümü var | - | bitstream bulundu (bkz. fpga/) |
| **Wi-Fi / BT** | PCI0 0xF8A00000 (len 0x500000), RP1; BT: UART + qcbtuart | PCIe düğümü **yok**; BT düğümü var | README (Lumia950XLPkg): PCIe RP1 firmware tarafından init, `pci-host-ecam-generic` önerisi | Drivers/bdwlan30.bin, qwlan30.bin, rampatch_tlv_3.x.tlv, nvm_tlv_3.x.bin |
| **Modem** | RFS0 (rmtfs), AMSS, QMUX, IPA; MPSS imajı | **yok** | rmtfs + qrtr | System32/qcdsp28994.mbn (+qcvss8994.mbn, qccpe8994.mbn) |
| **Ses** | ADSP + SLIMbus (SLM1 0xFE12F000, SLM2 0xFE1AF000), kodek I2C2 0x40 | **yok** | downstream common/audio.dtsi | System32/qcadsp8994.mbn |
| **Kamera** | CAMP/CAMS/CAMF/CAMT, VFE0, JPGE (0xFDA0xxxx) | **yok** | downstream mmo/common içinde kamera dtsi yok (qcom/ tarafında aranmalı) | - |
| **Batarya/şarj** | PMIC FG (UEFI PmicDxe), PMBM/PMBT | kısmi (PMIC var) | downstream common/charger.dtsi; battery/microsoft_bvt4d_3340mah.dtsi (950 XL pili 3340 mAh, bvt4d eşleşmesi dosya adından çıkarım) ve bvt5e_3000mah | provision: plat/PROVISION/BATTERY.PROVISION |
| **Haptics / LED / sensörler / NFC** | HWN0/1 (qchwnhaptics, qchwnled), SEN1, I2C çevre birimleri | **yok** | downstream common/{haptics,sensors,nfc,silabs}.dtsi | - |

## Panel (Sergej) açma dizisi (downstream = UEFI ile uyumlu, DCS)
11 (sleep out, 5 ms) → F0 5A 5A (level-2 unlock) → F2 63 → BD 11 01 02 16 02 16 → F0 A5 A5 (lock) → 53 20 → 51 80 → 35 00 (TE on) → 29 (display on).
Kapatma: 28, 10. Duke varyantında 11 sonrası bekleme 120 ms ve BD/FF dizileri farklı. Dosyalar: panel/panels.json, panel/panel_config_raw.xml.

## Önerilen sıra (gerçek port işi)
1. cityman için mainline DT yaması: usb3 etkinleştir + FPGA yükleme (bitstream UEFI'den) + MDSS/DSI/Sergej paneli.
2. PCIe (Wi-Fi) ve GPU node'ları: talkman için Android4Lumia950/linux fork'undaki çalışan eşdeğerlerden uyarla.
3. Firmware çıkarıcı betik: kurulum sırasında telefonun kendi UEFI/MainOS bölümünden firmware'i çıkarsın (repoda dağıtılmayacak).
4. ztOS cihaz katmanı: ztconflo'ya `uefi-esp` flash yöntemi + ztOSIAL manifest.

## Açık sorular
- FPGA bitstream sınırı sıfır dolguya göre tahmin edildi; "calibration file" ayrı bir kaynak, yeri bilinmiyor.
- Hangi panelin (Sergej/Duke) kullanıldığı, UEFI'nin DCS ID okumasına bağlı olabilir; cityman downstream dtsi yalnızca Sergej içeriyor.
- Mainline MDP5/DSI 20nm PHY'nin msm8994 için ne kadar hazır olduğu doğrulanmadı.
