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
