# Adreno 430 güç sırası analizi: Windows 10 Mobile + WOA + diğer çekirdekler

English version: [`en/GPU_DRIVER_ANALYSIS.md`](en/GPU_DRIVER_ANALYSIS.md)

Amaç: GPU'yu (Adreno 430) açarken SoC'yi resetleyen/kilitleyen SMMU ataması sorununu çözmek için üç kaynağın güç/saat sırasını karşılaştırmak. Sırayla: (1) orijinal Windows 10 Mobile (FFU) sürücüleri, (2) WOA-Project/Lumia-Drivers, (3) diğer çekirdekler (downstream MMO msm-3.10, fork, ana hat). Tescilli ikili dosyalar repoya konmadı; yalnızca bulgular yazıldı.

## 1. Windows 10 Mobile (FFU)

- GPU tek bir ACPI aygıtı: `GPU0` (QCOM246F). Ekran + Adreno 430 + Venus aynı aygıtta. Güç sıralamasını DSDT'deki PEP bloğu yürütüyor (`QCpep8994.sys`).
- GPU SMMU = MMU3 (`0xFDB10000`, 0xB000 boyut, IRQ 0x46 0x111 0x112 0x110, 3 bağlam bankası, `S2CB` 3 girdi). Sürücü `qcsmmu8994.sys`; **SCM/TZ çağrısı yok**, kayıtlar doğrudan programlanıyor.
- GPU çekirdek saati RPM kaynağı `/clk/oxili` (gfx3d_clk_src, RPM mem-clk tipi, id 1); GX rayı PMIC istemcisi `rail_gfx`.
- Açılış sırası: `/clk/oxili` = 19,2 MHz → GX GDSC (VDD_OXILI) → CX GDSC (VDD_OXILICX) → veri yolu oyu (APPSS→GFX3D_CFG) → `oxili_gfx3d_clk`, `oxilicx_ahb_clk`, `oxili_rbbmtimer_clk`. Kapanış tersi (rbbmtimer, cx_ahb, gfx3d kapat; yol oyu 0; GX sonra CX kapat; `/clk/oxili` = 0). Reset çevresinde assert/deassert (0xA/0xB, 1 µs).
- Mikrokod sürümleri linux-firmware'inkinden farklı (pm4 0x4ff062 / pfp 0x4ff085, linux-firmware 0x4ff065 / 0x4ff087): sorun değil, yalnızca sürüm farkı.

## 2. WOA Lumia-Drivers

- ARM32 sistem dosyaları FFU ile birebir aynı. ARM64 sürümü eski, yerel AArch64 derlemesi; disassembly için daha rahat. Sıra yukarıdakiyle aynı; ek bilgi yok. "Kusursuz değil" notu: GPU yalnızca ekran yolu için çalışıyor, 3B hızlandırma tam değil.

## 3. Diğer çekirdekler

- **Downstream MMO msm-3.10:** `kgsl_pwrctrl`: önce GX sonra CX rayı, saatler, AXI oyu, ardından A4xx SP/TP güç açma (`RBBM_POWER_CNTL_IP = 0x778000`, `SP_TP_PWR_ON` beklenir). GPU SMMU (`kgsl_iommu`) için de GX ve CX birlikte açılıyor (`__enable_regulators`, alt-vdd). `gdsc_enable` SW_COLLAPSE'ı bırakmadan önce kök saati açıyor, mantık-dışı yolda saat reset'lerini bırakıyor. v2.1 üst yaması: chipid `0x04030002`, tek seviye 630 MHz.
- **Bizim ağaç / fork:** `oxili_gx_gdsc` ve `oxili_cx_gdsc` mmcc'de tanımlı (`0x4024`/`0x4034`), GX için `vdd-gfx` besleme var. Fork'un mmcc "GPU workaround"u GX+CX'i SMMU'dan önce açıyor (`msm8994_oxili_pre_gpu_power`), GX'te kök saat 300 MHz'e ayarlanıyor (3.10 `initial-pwrlevel 2`). rpmpd `gfx_s2b_corner` için ACTIVE yanında SLEEP oyu da veriyor (3.10 `qcom,set = <3>`). a4xx_gpu.c: A430 SP/TP açılışı `a4xx_pm_resume`/`a4xx_submit` içinde (sınırsız `do/while` bekleme döngüsü var, zaman aşımı yok).
- **Ana hat:** A430 desteği yok; yalnızca fork'taki e8d7ff26a8f ile.

## 4. Bizim DT ile farklar (SMMU kilitlenmesinin olası nedenleri)

1. `gpu_iommu` düğümü yalnızca `OXILI_CX_GDSC` ile besleniyor; Windows ve downstream **GX'i de** açıyor. GX kapalıyken bağlam bankası erişimi → kilitlenme/reset.
2. DT'de 2 bağlam bankası ve ikisi de SPI 241; donanımda 3 banka (IRQ SPI 240/241/242, üçüncüsü güvenli, secure-id 18).
3. Windows TZ çağrısı yapmıyor; `restore_sec_cfg(18)` yalnızca downstream yolu olabilir (doğrulanmadı). MDP için çözüm 0034'te: gerçek bir etki alanı bağlanana kadar ertele.
4. Saat sırası: kök saat (`/clk/oxili`, RPM) önce 19,2 MHz, sonra GDSC'ler. Bizde kök saat 300 MHz ile GX'ten önce açılıyor; fark küçük ama Windows sırasını izlemek güvenli.
5. `a4xx_pm_resume` içindeki sınırsız bekleme döngüsüne zaman aşımı eklenmeli.

## 5. Önerilen donanım planı (henüz denenmedi)

1. DT: `gpu_iommu`'ya 3. bankayı ve IRQ'ları ekle, `power-domains` = GX + CX; düğümler yine `disabled` kalsın.
2. `ztdt.ko` ile çalışma zamanında yalnızca GX+CX aç, `GDSCR` durum bitlerini oku (SSH, SMMU'ya dokunmadan).
3. Başarılıysa GPU SMMU'yu (TZ'siz) bağla; sonra `msm` GPU düğümünü; `kmscube`/`glmark2-es2`.
4. Her adımdan önce telefon yeniden başlatma amacı kullanıcıya söylenecek.

## 6. Donanım deneyleri (2026-11-10, rescue, `ztgpupwr.ko` / `ztgfxrail.ko`)

| Deney | Sonuç |
|---|---|
| GX + CX GDSC elle aç (fork'un `msm8994_oxili_pre_gpu_power`) | Çalışır: GDSCR `a0282000` (PWR_ON, SW_COLLAPSE=0), telefon yaşıyor |
| + üç saat (gfx3d 300 MHz, cx_ahb, rbbmtimer) | CBCR'lerde CLK_OFF temiz (`0x4028=00006ff1`, `0x403c=20008001`, `0x40b0=1`) |
| + `rpmpd gfx` köşe oyu (durum 3 = svs_soc) | `gfx` domain açık, PM8994/pmi8994 S2 kullanımda |
| pmi8994 S2/S3 (VDD_GFX) SPMI dökümü | Açılışta zaten etkin (`enable 80`, `status1 81`): ray eksikliği sebep değil |
| GPU SMMU `ID0..2` oku (`smmu=1`) | **CPU donar** (reset değil, ekran ~183 sn'de kalır, USB ölür) |
| GPU `0xfdb00000` reg0..3 oku (`gpu=1`), ray oyu dahil | **CPU donar** |

Sonuç: Windows/downstream'de belgelenen güç, ray ve saat önkoşullarının hepsi sağlanıyor, ama hem GPU hem GPU SMMU kayıt alanına ilk erişim yine donuyor (cevap vermeyen bir slave). Elenmeyen adaylar: (a) Windows'taki `APPSS→GFX3D_CFG` yapılandırma yolu oyu (mainline interconnect'te karşılığı yok: `slv_cnoc_mnoc_mmss_cfg`/`slv_mmss_clk_xpu_cfg` var ama GFX3D_CFG düğümü yok), (b) XPU/MPU koruması (`slv_mnoc_mpu_cfg`; doğrulanmadı, daha önce bir reset yanlışlıkla XPU'ya yorulmuştu), (c) `qcdxkm8994.sys`'in ilk kayıt okumasından önce yaptığı bir adım (TZ/SMMU init ya da GPU bloğu reset'i). Sonraki adım canlı deneme değil, `qcdxkm8994.sys` (WOA ARM64 sürümü) içinde ilk RBBM erişimine kadarki kodun çevrimdışı çözümlenmesi.

### 6a. Ek deneyler (aynı gün)

- PEP yeniden okundu (`gpu_pep.txt`): güç açılışı = `/clk/oxili`=0x4B00 (19,2 MHz) → GX → CX → `BUSARB APPSS_PROC→GFX3D_CFG 0x11E1A300 (=300000000)` → üç saat. Ayrı bir `PSTATE 1`: üç saat için reset assert (0x0A), 1 µs, deassert (0x0B).
- `GFX3D_CFG` oyu RPM'e (`bmas` master 0 / `bslv` slave 11, bw 300000000) gönderildi: RPM **"resource does not exist" (-ENXIO)** döndü; bu telefonun RPM'inde veri yolu kaynakları yok (sürücüde her yerde `ignore_enxio` olması bunu açıklıyor).
- RPM `mmssnoc_ahb_clk` (MMSS NoC AHB kökü) hiç oylanmamıştı; oylandı (kullanıcı 1). **Sonuç: GPU reg0 okuması yine CPU'yu dondurdu.**
- `qcdxkm8994.sys`'te `GpuResetValue`/`BusResetValue` donanım reset'i değil, zamanlama (timeout) ayarları.
- Kalan adaylar: saat reset darbesi (PSTATE 1) ve kök saatin Windows'taki gibi 19,2 MHz ile başlaması, XPU/MPU, `qcdxkm8994.sys` içindeki başka bir ön adım.

## 7. Çalışan Android kaynaklarının incelenmesi (giannoccarol cityman, Android4Lumia950 lk2nd, mainline, marlin)

- **Android (cityman, kgsl) sırası bizimkiyle aynı:** `pwrlevel_change` → GX (`gdsc_oxili_gx`, `parent-supply = pmi8994_s2_corner`, RPM-smd-regulator, `qcom,set=3`) → CX → saatler (`core`, `iface`, `rbbmtimer`) → AXI oyu → A430 SP/TP (`RBBM_POWER_CNTL_IP = 0x778000`). kgsl'in kendisinde **SCM/TZ çağrısı yok**; yapılandırma yolu (GFX3D_CFG) oyu da yok (yalnızca veri yolu 26→512). Yani Windows'a özgü `APPSS→GFX3D_CFG` oyu Android'de gerekmiyor.
- **SMMU (`msm_iommu-v1`) ilk bağlama sırası:** regülatörler (CX + alt-GX) → `apply_bus_vote` (26→512) → saatler → SCM `restore_sec_cfg(18)` → BFB → bağlam. Kayıt erişimi TZ çağrısından **sonra**.
- **SMPB2 (pmi8994 S2/S3, VDD_GFX) Android'de RPM'e ait** (`rpm-regulator-smpb2`, köşe oyu). Bizim DT bunu aynı zamanda SPMI regülatörü (`vdd_gfx`, 980 mV) olarak da modelliyor. İki sahip çakışıyor olabilir; rpmpd köşe numaralandırması (downstream `init-voltage-corner = <2>` yorumda SVS SOC, enum NONE=1…) eşleşmesi doğrulanmadı.
- **lk2nd (msm8994):** GPU'ya özel başlatma yok; yalnızca `scm_xpu_err_fatal_init()` (XPU ihlallerini ölümcül yapar) ve MMSS AXI saatleri. Windows UEFI → GRUB yolunda aynı TZ/XPU durumu, ama XPU ihlalinin nasıl göründüğü (donma/reset) bu SoC'de doğrulanmış değil.
- **Mainline v7.3.1 `a4xx_gpu.c`:** 22894 bayt, `master` ile aynı; A430 güç kodu yok. Fork'un e8d7ff26a8f yaması dışında yeni bilgi yok. freedreno deposu yalnızca kullanıcı alanı araçları (`a4xx.xml.h`), çekirdek güç sırası içermiyor. marlin `adreno.h` yalnızca özellik bayrakları (`ADRENO_SPTP_PC` vb.).
- **Sonuç:** Android ile aynı sırayı uyguladığımız halde GPU kaydı donuyorsa fark platform durumunda ya da SMMU/SCM önkoşulunda. Sıradaki mantıklı deney, modül dürtmek yerine Android'in yaptığı gibi gerçek yolu izlemek: `restore_sec_cfg(18)` GX+CX+saatler açıkken (MDP'deki 0034 gibi, ama GPU SMMU için) ve SPMI/RPM çift sahipliğini ortadan kaldırmak.

## 8. Davitekk/ztOS `talkman-port` (Lumia 950, GPU çalışıyor) ile karşılaştırma

Kaynaklar: `Davitekk/ztOS` dalı `talkman-port` (`devices/microsoft-talkman`), `Android4Lumia950/linux` (sonic011gamer) GPU yamaları (`102627e`, `6b36f290`, `afc1c428`), `lk2nd`, `gus33000/boot-shim`.

- **Davitekk'te GPU için DT'de yalnızca `&gpu { status = "okay"; }` var**; çekirdek tabanı bizimkiyle aynı fork (`eb871dc8`). Yani DT yönünden eksik bir şey yok; fark bizim eklediğimiz **devre dışı bırakmalar** (`gpu`, `gpu_iommu`, `ocmem` kapalı) ve kendi deney modülümüzde.
- **Fork'un kendi yorumu kritik:** `qcom_iommu.c` içinde "*GPU SMMU is TZ-managed (secure-id 18) … TZ owns GR0, do not touch GR0*". GPU SMMU'nun **global alanı (GR0) TrustZone'a ait**; yalnızca bağlam bankaları (CB) ve `restore_sec_cfg(18)` sonrası erişilebilir. Benim `smmu=1` deneyim `0xfdb10020` (GR0 ID kayıtları) okuyordu: yanlış hedefti, donması beklenen davranış. 2026-11-09'daki "iommu group 0" resetini de aynı şey açıklıyor: `gpu_iommu` erken resume olunca `pre_gpu_voted()` yanlışken kod `qcom_iommu_reset_ns()` (GR0 yazımı) dalına düşüyor.
- **Gerçek sürücü yolunda sıra:** `msm8994_oxili_pre_gpu_power()` (GX→CX) → `pm_runtime_get_sync(gpu)` → SMMU tedarikçisi resume → `qcom_scm_restore_sec_cfg(18,0)` → ancak sonra GPU kayıtları. Deney modülümde `restore_sec_cfg(18)` **hiç çağrılmadı**; GPU çekirdek kaydına (0xfdb00000) da ondan önce dokundum. TZ'nin GFX3D XPU bölümünü bu çağrıyla açtığı olasılığı doğrulanmadı ama sıra Android ile (bölüm 7) uyumlu.
- **Önyükleme yolu:** Davitekk: UEFI → BootShim (AArch32; `ExitBootServices` sonrası SMC `0x0200010f` = SCM milestone 32→64, `el1_elr`=LK) → lk2nd → Android boot imajı. lk2nd `ENABLE_XPU_VIOLATION`'ı bilerek kapatır ("birincil yükleyici yapsın"); GPU'ya özel bir şey yapmaz, MMSS AXI saatleri yalnızca ekran için. Bizim yol UEFI → GRUB. TZ durumunun iki yolda farklı olup olmadığı **kaynaktan belirlenemedi**.
- **8992 vs 8994:** Davitekk'in çalıştığı donanım 8992 (A418 → A430 gibi). Fork'taki 8994 koşulu ("First hw_init works") sonic011gamer'ın 8994'te de ilk `hw_init`in çalıştığını söylüyor; bizde henüz gerçek yol hiç koşmadı.
- **Sonuç / plan (telefona dokunmadan):** (1) Bir daha ham MMIO okuması yok. (2) `gpu_iommu` için 8994 GPU dalında GR0'a **hiç** dokunmayan (`reset_ns` yok) ve MDP'deki `hw_deferred` gibi ilk gerçek `pre_gpu_power` sonrasına erteleyen yama. (3) DT'de `gpu`, `gpu_iommu`, `ocmem` açılıp gerçek sürücü yolu tek seferde denenir; SD günlüğü açık. Deney yalnızca kullanıcı bağlantı verdiğinde ve reboot nedeni önceden söylenerek yapılır.

## 9. Sonuç: GPU açıldı (2026-11-10, çekirdek `g140ac6e92bc9`, rescue FULL DT)

Yama (kernel/0038): GPU SMMU'da GR0'a dokunma yok, ilk `pre_gpu_power` öncesi donanım erişimi yok, bağlam programlaması `restore_sec_cfg(18)` sonrası, varsayılan alan identity. `gpu`, `gpu_iommu`, `ocmem` DT'de açık.
- Açılışta reset/donma **yok**; `fdb00000.gpu` iommu grubuna eklendi, a420 firmware yüklendi, `bound fdb00000.gpu (ops a3xx_ops)`.
- GPU `active`, CX GDSC açık, GX GDSCR PWR_ON (`0xA0282000`), `oxili_gfx3d_clk` açık.
- `debugfs dri/1/gpu`: `revision 430`, `rbbm-status 0x1`, ring `iova 0x2001000`, `rptr == wptr == 18` → CP başlangıç komutlarını işledi (hw_init tamam, SMMU çevirisi çalışıyor).
- Kalan: GPU iş yükü (Mesa freedreno a4xx) denemesi, soğutma cihazı hatası (`Couldn't register GPU cooling device`), devfreq 27 MHz'de boşta, `vdd/vddcx` regülatörleri dummy.
- Ders: 8. bölümdeki tahmin doğru çıktı; sorun güç oyları değil, TZ'ye ait GR0'a erken dokunmaktı.
