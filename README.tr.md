<div align="center">

# ztOS-950XL

**[ztOS](https://github.com/Zerone-Laboratories/ztOS) (Linux + ztMETRO kabuğu) için Microsoft Lumia 950 XL portu**

![durum](https://img.shields.io/badge/durum-erken%20bring--up-orange)
![cihaz](https://img.shields.io/badge/cihaz-Lumia%20950%20XL%20(cityman)-blue)
![soc](https://img.shields.io/badge/SoC-Snapdragon%20810%20(MSM8994)-red)
![kernel](https://img.shields.io/badge/kernel-mainline%20tabanl%C4%B1-success)
![boot](https://img.shields.io/badge/boot-UEFI%20%2B%20SD%20kart-lightgrey)

[English](README.md) · [Türkçe](README.tr.md)

</div>

> **Durum: erken bring-up.** Çekirdek telefonda SD karttan açılıyor ve **ztOS kök dosya sistemi (ztMETRO kabuğu) kartın 2. bölümünden çalışıyor**; **Adreno 430 GPU** ile çiziliyor (Mesa freedreno, arayüz 60 fps). Panel `drm/msm` ile çalışıyor (1440x2560), USB üzerinden SSH ile kurtarma kabuğu ve SD kart çalışıyor, Cortex-A57 kümesi ve CCI açılışta hızlandırılıyor (960 / 538 MHz; bootloader varsayılanı 384 / 269 MHz) ve SoC sıcaklık sensörleri thermal zone olarak görünüyor. Kamera, ses ve Wi-Fi henüz çalışmıyor. Ekran reset'inin kök nedeni: [`docs/research/MDP_SMMU_RESET.md`](docs/research/MDP_SMMU_RESET.md) (İngilizce). Tüm kronoloji: [`docs/research/BRINGUP_LOG.md`](docs/research/BRINGUP_LOG.md) ([English](docs/research/en/BRINGUP_LOG.md)). Açık işler: [`docs/OPEN_ITEMS.md`](docs/OPEN_ITEMS.md). Tam eMMC yedeği almadan bu depodaki hiçbir şeyi tek cihazınıza yüklemeyin.

## Hedef

Lumia 950 XL (RM-1085) üzerinde gerçek bir Linux telefon işletim sistemi çalıştırmak, Windows'a dokunmadan: **SD karttan** açılış, eMMC'ye (Windows 10 Mobile / Windows on ARM) dokunulmaz. Yeterince kararlı olursa LumiaWOA gibi genel bir yüklenebilir imaj olarak paylaşmak.

## Neden mümkün

| Parça | Kaynak |
|---|---|
| Kullanıcı alanı, kabuk, uygulamalar | ztOS (GSI + `ztOSIAL` cihaz katmanı) |
| Önyükleyici | Bootloader'ı açık telefon + Windows-on-ARM UEFI (Lumia950XLPkg) |
| Çekirdek tabanı | Mainline Linux + [Android4Lumia950/linux](https://github.com/Android4Lumia950/linux) çalışmaları (ekran, GPU, kamera, USB-C, şarj, ses) |
| Donanım bilgisi | Resmi Windows 10 Mobile FFU'sunun çevrimdışı analizi (ACPI, sürücüler, firmware, panel ve kamera ayarları) |
| GPU | `drm/msm` (Adreno 4xx) + Mesa `freedreno` — Windows sürücüsü kullanılmaz |

## Mimari

```
SD kart  ──►  Windows-on-ARM UEFI (eMMC'de, dokunulmaz) ──► rEFInd
                 │  SD üzerindeki GRUB (\EFI\BOOT\BOOTAA64.EFI, /ztos/grub.cfg)
                 ▼
           Linux çekirdeği (mainline + Lumia yamaları) + cityman DTB   (acpi=no, device tree)
                 │
                 ▼
           kurtarma initramfs'i (USB gadget + SSH, SD günlükçü, CPU/CCI hızlandırma)
                 │  ztos.root=LABEL=ZR_ARCH → switch_root
                 ▼
           SD'nin 2. bölümünde ztOS kökü (Arch Linux ARM, ztMETRO, ztOSIAL)
```

## Donanım durumu

Açıklama: ✅ telefonda çalışıyor · 🟡 kısmen çalışıyor / iş var · 🔴 çalışmıyor ya da kapalı · ❔ donanımda test edilmedi

| Alt sistem | Durum | Not |
|---|---|---|
| eMMC / SD | ✅ | eMMC görünüyor (HS200, dokunulmadı). SD çalışıyor; `no-1-8-v` ile 3.3 V yüksek hız (sabit 2.95 V vqmmc). |
| Dokunmatik (Synaptics RMI4) | ✅ | S3708AR giriş aygıtı olarak kayıt oluyor. |
| Ekran (çift DSI, komut modu) | ✅ | MDP5 v1.9 + çift DSI + Sergej panel, 1440x2560, konsol panelde. 4/4 temiz açılış. MDP SMMU'nun TZ kurulumu, DRM UEFI framebuffer'ını devralana kadar erteleniyor (yama 0034). DRM açılırken bir kez `pp done time out` kalıyor. |
| CPU saatleri | 🟡 | Henüz cpufreq sürücüsü yok. Açılışta `ztboost.ko` (kurtarma initramfs'i) PM8994 S11 rayı 1,0 V iken A57 kümesini 960 MHz'e (384'ten) ve CCI'yi 538 MHz'e (269'dan) çıkarıyor; A53 763 MHz'de kalıyor. Çok çekirdekli yükte SoC A57 saatini kendisi düşürüyor (4 çekirdek ~862 MHz). CPU 2,0 GHz hız bin'i (sigorta `0xFC4BC0B0`); Windows'un kendi frekans/voltaj tabloları (A57 1958 MHz'e kadar, tavan 1225 mV) çıkarıldı. DDR/BIMC saati hâlâ yönetilmiyor (DRAM gecikmesi ~215 ns). |
| Sıcaklık sensörleri | ✅ | TSENS v0.1, 16 sensör (yama 0037, kalibrasyon QFPROM'dan): cpu0-7 ve gpu için thermal zone. Yazılım renderlama ile boşta denecek sıcaklıklar: A53 62-67 °C, A57 65-77 °C. Yedek (redundancy) sigorta bölgesi ele alınmadı. |
| ztOS kullanıcı alanı | 🟡 | Kök SD'nin 2. bölümünden açılıyor (systemd ayakta, `zerone` ile SSH, ztOSIAL servisleri). ztMETRO Mesa `freedreno` ile GPU'da çiziliyor (`/etc/ztos/session.env`), 60 fps akıcı. GPU öncesi: llvmpipe, 3-9 fps. |
| GPU (Adreno 430) | ✅ | `drm/msm` + Mesa `freedreno` (`FD430`, OpenGL ES 3.0). Yama 0038: GPU SMMU'nun global alanı (GR0) TrustZone'a ait; ilk GX/CX güç açılışı ve `restore_sec_cfg(18)` öncesi erişim yok. Kök neden ve yol: [`docs/research/GPU_DRIVER_ANALYSIS.md`](docs/research/GPU_DRIVER_ANALYSIS.md). Firmware `a420_pm4/pfp.fw` linux-firmware'den. Açık: GPU cooling device, devfreq, `vdd`/`vddcx` regülatörleri. |
| Kamera | 🔴 | CAMSS ve VFE/Venus SMMU'ları aynı SMMU nedeniyle DT'de kapalı; CCI probe oluyor. CCS sürücüsü sensörlerle daha önce çalışmamıştı. |
| USB gadget (kurtarma/SSH) | ✅ | DWC3 + HS PHY (TCSR yönlendirme + LK QSCRATCH değerleri). Windows ACM + RNDIS görüyor, SSH çalışıyor. |
| Şarj / fuel gauge | 🟡 | SMBCHG ve PMI8994 FG probe oluyor (pil ID okunuyor); init arka planda (tek SRAM izni, ~1 sn); kapasite, gerilim, akım ve sıcaklık doğru okunuyor. |
| Wi-Fi / Bluetooth | 🔴 | PCIe1 (Wi-Fi) DT'ye bağlı değil; LineageOS cityman portunda `GCC_PCIE_1_BCR` reset'i ve Android QCA6174 `qwlan30.bin` gerekiyor. BT sürüm okuması zaman aşımı veriyor. |
| Ses | 🟡 | Ses kartı henüz yok: `msm-snd` ertelenmiş. İmzalı ADSP imajı orijinal FFU'nun kendi `System32\qcadsp8994.mbn` dosyası (LineageOS cityman portundakiyle aynı hash); kullanıcının kendi telefonundan alınmalı. |
| Modem / SIM | 🔴 | ADSP yükleniyor, modem yüklenmiyor (upstream notlarına göre). |
| Sensörler, titreşim, NFC | 🟡 | Hall, tuşlar, barometre, manyetometre çalışıyor; MPU6050/ışık sensörü yanıt vermiyor. NFC DT'de kapalı (arka kapak yokken polarite tespiti 33 sn bloklar). |

## Yol haritası

- [x] Fizibilite çalışması ve ztOS mimari incelemesi
- [x] Orijinal FFU'nun çevrimdışı çıkarımı ve analizi (ACPI, firmware envanteri, panel, kamera)
- [x] Çatal `master` + `camss` benzersiz parçaları tek ağaçta birleştirildi; çekirdek + cityman DTB derleniyor
- [x] `usb3` + UC120'yi etkinleştir → USB gadget kurtarma kabuğu (SSH çalışıyor)
- [x] SD karttan ilk cihaz üstü açılış (efifb konsolu)
- [x] Ekran: msm DRM ile Sergej paneli, açılış SMMU reset'i çözüldü
- [x] Açılış süresi: init ~3 sn (fuel gauge init açılış yolundan çıkarıldı)
- [x] SD karttaki ztOS kök dosya sistemi açılıyor (yazılım renderlama)
- [x] A57 / CCI saatleri açılışta yükseltiliyor; CPU hız bin'i ve Windows frekans/voltaj tabloları bulundu
- [x] TSENS sıcaklık sensörleri (yama 0037)
- [ ] Kalan ekran uyarıları
- [x] GPU: yama 0038 ile açıldı (sıradaki: `glmark2-es2`, devfreq, soğutma)
- [ ] Sıcaklık izlemeli kademeli A57 artışı, 787 MHz üstü A53, DDR/BIMC saati, gerçek cpufreq sürücüsü
- [ ] `ztconflo` `uefi-esp` flash yöntemi + ztOSIAL manifest
- [ ] Wi-Fi, ses, şarj, sensörler
- [ ] Kamera: CCS sensör bring-up, ardından AF/OIS/flaş, libcamera
- [ ] Genel yüklenebilir imaj / yükleyici

## Depo yapısı

```
kernel/            Android4Lumia950/linux master üstüne yama serisi 0001-0037 (0029/0030 ekle-geri al, 0033 geçici hata ayıklama; tarihçe olarak duruyor), kernel .config, derlenmiş DTB'ler
rescue/            Kurtarma initramfs'i (init, USB gadget, SSH, SD günlükçü, açılışta CPU/CCI hızlandırma), GRUB menüsü (FULL DT rescue, SAFE DT rescue, ztOS (varsayılan), Windows, önceki çekirdek yedeği), SD/ESP betikleri
rescue/tools/      Hata ayıklama ve açılış modülleri: ztboost.c (açılışta A57/CCI saatleri), ztcpu.c / ztcci.c (saat deneyleri), ztfuse.c / ztfuse2.c (hız bin'i ve TSENS kalibrasyon sigortaları), ztts.c (elle TSENS okuma), ztspmi.c (PM8994 buck dökümü), ztdt.c (DT düğümünü çalışma anında açar), ztsec.c (TZ güvenli sayfa tablosu SCM çağrıları)
docs/research/     Bulgular (bring-up günlüğü, donanım boşlukları, GPU/kamera raporu, çatal analizi, ACPI cihaz haritası, kamera ayarları); Türkçe aslı ve `docs/research/en/` altında İngilizce sürümleri
```

Başlangıç için [`docs/research/MDP_SMMU_RESET.md`](docs/research/MDP_SMMU_RESET.md), [`docs/research/PORTING_GAPS.md`](docs/research/PORTING_GAPS.md) ve [`docs/research/SOURCES.md`](docs/research/SOURCES.md) dosyalarına bakın. Her araştırma notunun Türkçe aslı ve İngilizce sürümü (`docs/research/en/`) var.

## Çekirdeği derleme

```sh
git clone https://github.com/Android4Lumia950/linux && cd linux
git checkout master
git am /yol/ztos-950xl/kernel/00*.patch
cp /yol/ztos-950xl/kernel/ztos-lumia.config .config   # NR_CPUS=8, DRM_MSM=y, QCOM_WDT=y, izleme açık
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j"$(nproc)" Image qcom/msm8994-msft-lumia-octagon-cityman.dtb
```

## Güvenlik

- Herhangi bir şey yapmadan önce **tam eMMC yedeği** alın.
- EFS, MODEM, SBL, TZ ve RPM bölümlerine asla dokunmayın.
- SD'den açılış Windows'u korur, ama kötü bir çekirdek telefonu yine de kilitleyebilir; flaş moduna girmenin bir yolunu hazır tutun.

## Hukuki

- Analiz, yazarın kendi cihazının yazılımı üzerinde, birlikte çalışabilirlik amacıyla yapıldı.
- **Hiçbir Microsoft / Qualcomm / Lattice firmware'i ya da ikili dosyası depoya konmaz.** Tescilli parçalar kurulum sırasında telefonun kendisinden çıkarılmalıdır.
- Çekirdek yamaları GPL-2.0-only (Linux çekirdeğiyle aynı). Araştırma notları atıfla serbestçe kullanılabilir.

## Teşekkürler

- [Zerone Laboratories](https://github.com/Zerone-Laboratories) — ztOS / ztMETRO
- [Android4Lumia950](https://github.com/Android4Lumia950) ve katkıcıları — bu işin dayandığı Lumia 950 / 950 XL mainline çekirdek çalışmaları (ekran, GPU, CAMSS, UC120, SMBCHG, ses)
- Sony/LineageOS MSM8994 3.10 çekirdekleri — açılış reset'ini açıklayan downstream SMMU ve MDSS kodu
- giannoccarol (Lorenzogianno) — LineageOS 23.2 cityman portu ve belgeleri; Empyreal96, wiki.nura.eco ve Davitekk — notlar ve USB PHY değerleri. Tam liste: [`docs/research/SOURCES.md`](docs/research/SOURCES.md)
- LumiaWOA / Lumia950XLPkg topluluğu — UEFI ve yıllarca biriken tersine mühendislik bilgisi
- Linux, Mesa/freedreno ve postmarketOS toplulukları

## Katkı

Issue ve PR'lar memnuniyetle karşılanır — özellikle donanım test raporları. Lumia 950 XL, 950 ya da seri/USB hata ayıklama düzeneğiniz varsa bir issue açın.
