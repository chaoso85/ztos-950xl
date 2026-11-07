<div align="center">

# ztOS-950XL

**[ztOS](https://github.com/Zerone-Laboratories/ztOS) (Linux + ztMETRO kabuğu) için Microsoft Lumia 950 XL portu**

![durum](https://img.shields.io/badge/durum-erken%20ara%C5%9Ft%C4%B1rma-orange)
![cihaz](https://img.shields.io/badge/cihaz-Lumia%20950%20XL%20(cityman)-blue)
![soc](https://img.shields.io/badge/SoC-Snapdragon%20810%20(MSM8994)-red)
![kernel](https://img.shields.io/badge/kernel-mainline%20tabanl%C4%B1-success)
![boot](https://img.shields.io/badge/boot-UEFI%20%2B%20SD%20kart-lightgrey)

[English](README.md) · [Türkçe](README.tr.md)

</div>

> **Durum: erken araştırma.** Burada hiçbir şey henüz telefonda açılmadı. Çekirdek derleniyor; geri kalanı analiz, plan ve orijinal yazılımdan çıkarılan veriler. Tam eMMC yedeği almadan bu depodaki hiçbir şeyi tek cihazınıza yüklemeyin.

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
SD kart  ──►  Windows-on-ARM UEFI (eMMC'de, dokunulmaz)
                 │  GRUB / EFI stub
                 ▼
           Linux çekirdeği (mainline + Lumia yamaları) + cityman DTB   (acpi=no, device tree)
                 │
                 ▼
           ztOS GSI (ztMETRO)  +  cihaz katmanı (/vendor/ztos/manifest.toml, ztOSIAL)
```

## Donanım durumu

Açıklama: ✅ derleniyor / veri hazır · 🟡 çatalda kod var, temizlik gerek · 🔴 başlanmadı · ❔ donanımda test edilmedi

| Alt sistem | Durum | Not |
|---|---|---|
| eMMC / SD | ✅ ❔ | Mainline DT'de (HS200). |
| Dokunmatik (Synaptics RMI4) | ✅ ❔ | Mainline DT'de. |
| Ekran (çift DSI, Samsung-AMOLED tarzı komut modu) | 🟡 ❔ | Panel `Sergej`; init dizisi UEFI ve downstream ile çapraz doğrulandı. 20nm DSI PHY ve MDP5 çift-LM yamaları "gözden geçirilecek". |
| GPU (Adreno 430) | 🟡 ❔ | `drm/msm` tanıyor (chip id `0x04030002`). Çatalda kirli güç yönetimi geçici çözümleri var, kök nedene indirilmeli. |
| Kamera | 🟡 ❔ | DT'de CAMSS 8x94 + CCI + 3 SMIA++ sensör. Sensör sürücüsü (CCS) henüz çalışmıyor. |
| USB gadget (kurtarma/SSH) | 🟡 ❔ | `usb3` + Lattice UC120 Type-C kodu var; etkinleştirilip test edilmeli. **İlk açılış engeli.** |
| Şarj / fuel gauge | 🟡 ❔ | SMBCHG + PMI8994 FG yamaları "gözden geçirilecek". |
| Wi-Fi / Bluetooth | 🔴 | PCIe RP1 (Wi-Fi) DT'ye bağlı değil; BT düğümü var. |
| Ses | 🟡 ❔ | WCD9330 / TAS2552 / ADSP yamaları, "sonra temizlenecek". |
| Modem / SIM | 🔴 | ADSP yükleniyor, modem yüklenmiyor (upstream notlarına göre). |
| Sensörler, titreşim, NFC | 🔴 | Veriler `docs/research/DEVICES.md` içinde. |

## Yol haritası

- [x] Fizibilite çalışması ve ztOS mimari incelemesi
- [x] Orijinal FFU'nun çevrimdışı çıkarımı ve analizi (ACPI, firmware envanteri, panel, kamera)
- [x] Çatal `master` + `camss` benzersiz parçaları tek ağaçta birleştirildi; çekirdek + cityman DTB derleniyor
- [ ] `usb3` + UC120'yi etkinleştir → USB gadget kurtarma kabuğu
- [ ] SD karttan ilk cihaz üstü açılış (efifb konsolu)
- [ ] Ekran + GPU: geçici çözümleri kaldır, düzgün runtime PM, `kmscube` / `glmark2-es2`
- [ ] ztOS GSI rootfs + `ztconflo` `uefi-esp` flash yöntemi + ztOSIAL manifest
- [ ] Wi-Fi, ses, şarj, sensörler
- [ ] Kamera: CCS sensör bring-up, ardından AF/OIS/flaş, libcamera
- [ ] Genel yüklenebilir imaj / yükleyici

## Depo yapısı

```
kernel/            Android4Lumia950/linux master üstüne yama, kernel .config, derlenmiş cityman DTB
docs/research/     Bulgular (donanım boşlukları, GPU/kamera raporu, çatal analizi, ACPI cihaz haritası, kamera ayarları)
```

Başlangıç için [`docs/research/PORTING_GAPS.md`](docs/research/PORTING_GAPS.md) ve [`docs/research/GPU_CAMERA_REPORT.md`](docs/research/GPU_CAMERA_REPORT.md) dosyalarına bakın. Araştırma notlarının çoğu Türkçe yazıldı.

## Çekirdeği derleme

```sh
git clone https://github.com/Android4Lumia950/linux && cd linux
git checkout master
git am /yol/ztos-950xl/kernel/0001-*.patch
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- mmo_defconfig
scripts/config -e DRM_PANEL_MICROSOFT_SERGEJ -e DRM_PANEL_MICROSOFT_DUKE -e VIDEO_CCS
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
- LumiaWOA / Lumia950XLPkg topluluğu — UEFI ve yıllarca biriken tersine mühendislik bilgisi
- Linux, Mesa/freedreno ve postmarketOS toplulukları

## Katkı

Issue ve PR'lar memnuniyetle karşılanır — özellikle donanım test raporları. Lumia 950 XL, 950 ya da seri/USB hata ayıklama düzeneğiniz varsa bir issue açın.
