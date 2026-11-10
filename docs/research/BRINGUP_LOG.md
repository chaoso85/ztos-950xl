# Cihaz üstü bring-up günlüğü (2026-11-07 … 2026-11-10)

English version: [`en/BRINGUP_LOG.md`](en/BRINGUP_LOG.md)

Bu belge, çekirdeğin ilk kez gerçek telefonda (Lumia 950 XL, cityman) çalıştırılmasından çıkan **doğrulanmış** bulguları toplar. Önceki araştırma notları (PORTING_GAPS, FINDINGS_feasibility) yalnızca çevrimdışı analizdi; burada cihazda ölçülenler var.

## 1. Açılış zinciri (çalışıyor)

```
stock UEFI (eMMC) -> rEFInd -> GRUB arm64-efi (SD: \EFI\BOOT\BOOTAA64.EFI) -> Linux 7.3-rc2 (Android4Lumia950 tabanı) + initramfs
```

- Windows (eMMC) değiştirilmedi; GRUB menüsünde "Windows (eMMC)" girişi `bootmgfw_real.efi`'ye zincirler.
- GRUB'da `loadenv/save_env` modülleri yok, yani tek seferlik açılış seçimi yapılamıyor; giriş 7 sn'lik menüden elle seçilir.
- Girişler (`rescue/grub.cfg`, güncel hâl 2026-11-10, `set default=2`): 0) FULL DT rescue (msm DRM panel, CPU hızlandırma açık), 1) SAFE DT rescue (MDSS, GPU, kamera ve SMMU'ları kapalı, UEFI framebuffer; `ztos.noboost` ile CPU hızlandırma atlanır), 2) ztOS (`ztos.root=LABEL=ZR_ARCH`, SD'nin 2. bölümü; varsayılan), 3) Windows (eMMC), 4) önceki çekirdek (`Image.prev`, TSENS öncesi) yedeği. Geçmiş değişiklikler aşağıdaki bölümlerde.

## 2. USB gadget: kurtarma kabuğu (doğrulandı)

Windows ACM (COM portu) + RNDIS adaptörünü görüyor; 192.168.77.1 <-> 192.168.77.2 ping ve yalnızca anahtarla SSH (dropbear) çalışıyor. Çözümler (hepsi cihazda ölçüldü):

| Sorun | Neden | Çözüm |
|---|---|---|
| Port DWC3'e gitmiyordu | TCSR 0xfd4ab000 | 1 yazılır (patch 0014) |
| Windows enumerasyonu takılıyor | LK'nın QSCRATCH değerleri yoktu | `qcom,lk-wrapper-regs` ile yeniden yazılıyor (patch 0015; değerler Davitekk/ztOS talkman portundan, cityman için tek tek doğrulanmadı) |
| Kod 10 | Boş seri numarası: configfs özniteliği `serialNumber` yazılmıştı | küçük harf `serialnumber` |
| RNDIS eşleşmiyordu | Microsoft OS tanımlayıcıları VID/PID/bcdDevice başına önbelleğe alınır | `bcdDevice` değiştirilir |
| RNDIS sürücüsü başlamıyordu | ACM önce bağlanmıştı | RNDIS önce (arayüz 0), ACM sonra |

ECM için Windows sürücüsü yok. Windows tarafında IP elle verilir (`phonenet.cmd`).

## 3. SD kart

`sdhc2` kartı UHS-I DDR50 ile açıyordu, ~60 sn'de bir `mmc2: Timeout waiting for hardware interrupt` (CMD6, ADMA Err 3) ve 3.3 V HS'ye düşüş. Sebep: `vqmmc-supply` sabit ~2.95 V (l13a), 1.8 V'a geçemez. Çözüm `no-1-8-v` (patch 0016).

Rescue init SD'yi `rw,sync,flush` bağlar. Kayıtlar yazılırken sert reset edilirse FAT kök dizini bozulabilir (bir kez oldu); bu yüzden `/sbin/safe-reboot` (kayıt işlerini durdurur, sync, umount ya da `ro` remount, ancak sonra reset; SD hâlâ rw ise reset etmez). Hâlâ her açılışta "Volume was not properly unmounted" görülüyor, nedeni bilinmiyor.

## 4. Tam DT (ekran/GPU) açılış günlüğü denetimi

3799 satırlık `dmesg` (initcall_debug + `keep_bootcon`) baştan sona okundu; "error" yazmayan sorunlar da listelendi. Not: `keep_bootcon` + efifb açılışı ~40 kat yavaşlattığından erken zaman damgaları şişiktir; saniyelerce süren beklemeler gerçektir.

### 4.1 Ekran ve GPU

| Bulgu | Anlamı |
|---|---|
| `pclk0/pclk1/byte0/byte1_clk_src: rcg didn't update its configuration` (8 WARN) | DSI açılırken mmcc RCG'leri ayarlanıyor ama parent'ları olan DSI PHY PLL çalışmıyor. `msm_mdp: pp done time out, lm=2` (sürekli) büyük olasılıkla buradan |
| `fb0: sys_imageblit / sys_fillrect: framebuffer is not in virtual address space` | fbcon, msm fbdev katmanına geçerken bozuk eşleme |
| `qcom/a420_pm4.fw` yok | sysfs fallback ~61 sn bekliyor, sonra GPU yine başlamıyor |
| `oxili_gfx3d_clk status stuck at 'off'` (7 WARN) + `Failed to enable clk '(null)': -16` | GPU iommu `qcom_iommu_resume`, GX güç alanı açılmadan bu saati açmaya çalışıyor; iommu probe'u ~28 sn sürüyor |
| DSI `lab_reg`/`ibb_reg`, adreno `vdd`/`vddcx` dummy regülatör | DT'de tanımsız |
| `Couldn't register GPU cooling device` | DT'de thermal bağlantısı yok |

### 4.2 Boot'u geciktiren, "error" yazmayan beklemeler

| Bekleme | Neden |
|---|---|
| ~33 sn | `pn544` NFC `nfc_en` polaritesini bulamıyor (arka kapak = anten takılı değil); I2C kontrolcü probe'unu tıkıyor. DT'de devre dışı bırakıldı (patch 0017) |
| ~19,8 sn | fuel gauge probe (pil ID 325 kΩ, beklenen 330 ±%15: sağlıklı) |
| ~8,3 sn | RPM regülatörleri (`regulators-0`), RPM `deferred_probe_initcall` ile geç açılıyor; hepsi "Bringing 0uV into ..." |
| ~5,9 sn | `tsl2772` (dummy supply, -110) |
| ~3,3 sn | vadc + sahte `spurious spmi irq received for apid=158` WARN |
| 60 sn | `qcadsp8994.mbn` yok (sonunda -110); `regulatory.db` yok |

### 4.3 DT'de olmayan ya da yanlış donanım

- `si470x 5-0011`: DeviceID/ChipID 0x0000 ama probe "başarılı" (orada çip yok)
- `fan53555 5-006c`: `Failed to get chip ID`
- `ad714x_captouch 4-002c`: `IRQ not configured` (-22)
- `inv-mpu6050 2-0068`: whoami 0x00, beklenen 0x70
- `ccs 6-0020 / 6-006c / 7-0020`: dummy supply, register yazılamıyor, CCI timeout; genel CCS sürücüsü bu sensörlere uygun değil. mmcc `sync_state()` bu üçünü bekliyor
- `i2c_qup f9967000`: tx DMA kanalı yok (PIO)

### 4.4 Çekirdek ve güç

- `Number of cores (8) exceeds configured maximum of 6`: config Lumia 950'den (2xA57+4xA53) kalan `NR_CPUS=6` taşıyordu; 950 XL 4xA57+4xA53. 8'e çıkarıldı; PMU düğümü A53/A57 olarak bölündü (patch 0017)
- `psci-cpuidle` 19 (-ENODEV) döndürüyor: CPU idle durumları yok
- `l2: bypassed regulator has no supply!`, `s2: Bringing 770000uV into 980000uV`
- Bluetooth ROME/QCA6390: 0xfc00 komutu üç kez timeout (çip cevap vermiyor)
- Ses: `MultiMedia1: error getting cpu dai name`, "No soundcards found" (ADSP firmware yok)
- `talkman: camera snapshot reboot armed on gpio-keys` (kamera tuşu yeniden başlatabilir)
- `clk/genpd/regulator: Not disabling unused ...` bilinçli (`*_ignore_unused`)
- ramoops kayıt altına alınıyor ama UEFI/GRUB belleği ezdiği için reset sonrası bir şey kalmıyor: pstore işe yaramaz

### 4.5 Çalışanlar

Synaptics S3708AR dokunmatik (RMI4), güç/resin tuşları, hall sensörü, barometre (zpa2326), manyetometre (ak8975), SD (yüksek hız), iç eMMC görünüyor (`mmcblk1`, 40 bölüm; dokunulmadı), USB, SMBCHG, MDP5 v1.9 + DSI + panel `sergej` bağlanıyor.

## 4.6 O günkü sıradaki iş listesi

1. DSI PHY PLL / piksel saati sırası (pp done time out)
2. GPU iommu saati / güç alanı + `a420_pm4.fw`, `a420_pfp.fw` initramfs'e
3. DT temizliği: si470x, fan53555, ad714x, mpu6050, ccs kapat; tsl2772 supply'ları
4. `keep_bootcon`'u kaldırıp zamanlaması gerçekçi günlük almak

## 5. 2026-11-08: FULL DT boot 998 s'de hâlâ init'e ulaşmadı — iki ayrı neden

1. **Gerçek kilitlenme (0019 kaynaklı):** 0019, GPU iommu'dan `tbu` (OXILI_GFX3D_CLK) clock'unu çıkardı. Referans
   `qcom_iommu_resume()` bind sırasında `clk_bulk_prepare_enable()`'ın bu clock yüzünden başarısız olmasına güveniyor
   ("clk_bulk still fails"); başarısız olunca SMMU'ya dokunmadan dönüyor. tbu kalkınca enable başarılı oldu, GX kapalıyken
   reset_ns/BFB yazmaları yapıldı ve CPU5 bus'ta kilitlendi: RPM regülatör probe'undan (139.7 s) hemen sonra RCU stall,
   init hiç başlamadı. 0020 tbu'yu geri koydu; `stuck at 'off'` uyarısı beklenen ve zararsız sonuç.
2. **Yavaşlık:** `keep_bootcon` efifb boot konsolunu açık tutuyor; satır başına ~80 ms (normalde ~1 ms). Eski FULL DT
   boot'u (#12) init'e ancak 312 s'de ulaştı. FULL DT girdisinden kaldırıldı, grub.cfg'de uyarı notu var; FULL DT artık
   varsayılan girdi.

SD düzeni: `\ztos\{Image, rescue.cpio.gz, grub.cfg, msm8994-msft-lumia-octagon-cityman.dtb, logs}`;
`update_sd.ps1` bunları yazar, hash + grub.cfg referanslarını doğrular, eski dosyaları siler.

## 6. 2026-11-09: FULL DT ilk temiz açılış (build #14 + 0020 DTB, keep_bootcon yok)

- 8 CPU, init 32.3 s, USB/SSH 34.6 s, SD log çalışıyor. **Ekran msm DRM ile 1440x2560 çalışıyor** (DSI-1 connected/enabled,
  fb0 = msmdrmfb, DSI0 VCO 1.446 GHz, pclk 120.49 MHz). a420 firmware initramfs'ten yüklendi.
- Boot süresinin çoğu: fuel gauge probe'u 19.7 s (3.0 → 20.7 s), deferred probe 8.3 s, RPM regülatörleri 2.3 s, GPU iommu 2.3 s,
  DSI PHY 2.8 s (uyarı trace'leri yazdırılırken), panel 1.0 s.
- 0018 yan etkisi: DSI PHY clock provider kaydında orphan RCG'ler yeniden ebeveynlenirken `__clk_set_parent_before()`
  CLK_OPS_PARENT_ENABLE için `clk_core_prepare_enable()` çağırıyor; PHY henüz açık olmadığı için VCO prepare başarısız, hata
  yok sayılıyor, `_after()` disable/unprepare yapınca 6 adet "dsi0pll(byte) already disabled/unprepared" WARN. Sayaçlar 0'da
  kalıyor (zararsız) ama düzeltilmeli.
- Diğer: `pp done time out, lm=2` (bir kez), `fb0: ... not in virtual address space` (konsol yine de çalışıyor),
  ad714x "platform data doesn't exist" -22 (sürücüde DT desteği yok → düğüm kapatılmalı), spurious SPMI irq apid=158,
  BT QCA 0xfc00 timeout ×4, qcadsp8994.mbn yok → ses kartı yok, regulatory.db yok, psci-cpuidle -ENODEV,
  `l2: bypassed regulator has no supply`, i2c_qup f9967000 tx DMA yok, rmi4 vio / zpa2326 vref / DSI lab/ibb / adreno vdd,vddcx dummy.
- Bu açılıştan önceki açılış kendiliğinden yeniden başladı; `dmesg-prev.txt` oluşmadı (init'e ulaşmamış) ve pstore boş
  (ramoops belleği yeniden başlatmada korunmuyor). Nedeni bilinmiyor; kamera tuşu da reboot tetikleyebiliyor (talkman).

## 7. 2026-11-09: 0021/0022 ve ADSP firmware notu

- 0021 (clk core): orphan yeniden-ebeveynlemede CLK_OPS_PARENT_ENABLE dansı atlanıyor → 0018'in 6 "already disabled" WARN'ı için.
- 0022: ad7147 (sideinteraction) düğümü kapatıldı; sürücüde DT desteği yok.
- SD'de yedek: `Image.prev` + `msm8994-msft-lumia-octagon-cityman.prev.dtb` ("PREVIOUS build" GRUB girdisi) = son çalışan build.
- ADSP: `qcadsp8994.mbn` Windows'un kendi imzalı dosyası (FFU: `mainos/Windows/System32`, 10 149 719 bayt, birleşik 32-bit ELF,
  19 segment, 0x0c800000–0x0e000000 = 24 MiB, segmentler relocatable). DT'deki `adsp_mem` (0x0ca00000, 24 MiB) boyut olarak
  uyuyor; `firmware-name` zaten Windows adıyla aynı. Dosya imzalı (TZ doğruluyor) → biz yazamayız/değiştiremeyiz, sadece
  kullanıcının kendi FFU'sundan alınıp kullanılabilir; depoya konmaz. Ses için ayrıca WCD9330 (SLIMbus), TAS2552 ve
  q6 APR/AFE yolu gerekiyor; `.acdb` kalibrasyon dosyaları mainline'da kullanılmıyor.

## 8. 2026-11-09: her girdi ~30 s'de reset (dünkü çalışan build dahil)

- Windows normal açılıyor, pil %70. SD'de `dmesg-prev.txt` yok → bugünkü açılışların hiçbiri init'e (32 s) ulaşmadı.
- Dün çalışan Image/DTB ile de reset olduğu için neden build'de değil; donanım/firmware durumuna bağlı bir şey.
- Hipotez (doğrulanmadı): firmware'in bazen kurulu bıraktığı APSS watchdog'u (f9017000). CONFIG_QCOM_WDT kapalıydı
  (referans mmo_defconfig'de de kapalı), yani kimse beslemiyordu. 0023: QCOM_WDT açık + probe'da EN/STS/BARK/BITE logu;
  qcom-pon probe'unda PMIC reset nedenleri (PON/WARM/POFF/SOFT) logu.

## 9. 2026-11-09: reset noktası videodan bulundu → GPU + GPU SMMU kapatıldı (0024)

- Video (28 s): son okunan satır 26.649 s `platform fdb00000.gpu: Adding to iommu group 0`, ardından yeni bir
  `oxili_gfx3d_clk stuck at 'off'` WARN'ı basılırken reset. Dünkü çalışan açılışta aynı yer 26.18 s ve geçmişti.
- `qcom_iommu_attach_dev()` `pm_runtime_get_sync()` hatasını yok sayıp GX kapalıyken GPU SMMU context bank'ını
  programlıyor; sonucu donanım durumuna göre değişiyor (dün geçti, bugün reset, PREVIOUS girdisinde siyah ekran + donma).
- SAFE DT (GPU + GPU SMMU kapalı) hiç bu sorunu yaşamadı. 0024 FULL DT'de &gpu ve &gpu_iommu'yu kapatıyor; ekran (MDP/DSI)
  GPU'ya ihtiyaç duymuyor. GPU zaten çalışmıyordu (GX rail tanımlı değil).
- SD: PREVIOUS girdisi = dünkü Image + yeni (GPU kapalı) DTB.

## 10. 2026-11-10: GPU kapalı FULL DT yine ~25.07 s'de reset → CAMSS/VFE/Venus SMMU kapatıldı (0025)

- Video (son 7 s, 10 fps + son 1.4 s 30 fps): gfx3d WARN yok (0024 çalıştı). Son satırlar: venus SMMU (fdc84000)
  ctx'leri + "returned 0 after 95671 usecs", ardından VFE SMMU ctx'leri (fda4c000/fda4d000) 25.068 s → reset.
  Dünkü başarılı logda bu noktanın hemen ardından `fda0ac00.camss: Adding to iommu group 1` (attach) geliyor.
- Aynı sınıf sorun: SMMU attach, blok güçsüzken context bank'ı programlıyor. SAFE DT bunların hepsini kapatıyordu.
- 0025: &camss, &vfe_iommu, &venus_iommu kapalı. Ekran için &mdp_iommu açık kalıyor; bu da reset verirse
  bir sonraki şüpheli o.

## 11. 2026-11-10: GPU + CAMSS/VFE/Venus SMMU kapalı FULL DT açıldı (#17, DTB 0025)

- init 25.37 s, USB/SSH 27.9 s; ekran msm DRM ile çalışıyor ("no GPU device was found" beklenen). DSI PHY artık 2.8 s değil
  (0021 sonrası WARN yok). ad714x satırı yok (0022).
- Watchdog: `EN=0x0 STS=0x0` → firmware APSS watchdog'u kurulu bırakmamış; watchdog hipotezi YANLIŞ çıktı.
- PMIC: `pon a0 warm 00 00 poff 08 00 soft 00 00`. Downstream qpnp-power-on tablosuna göre POFF bit3 = GP1
  (Keypad_Reset1, tuşla zorla kapatma); warm reset nedeni yok. Önceki döngü kullanıcının tuşla kapatmasıyla bitmiş.
- Kalanlar: fuel gauge 19.3 s, `pp done time out, lm=2` ×2, fb0 "not in virtual address space" (imageblit/fillrect),
  spurious SPMI irq apid=158, BT QCA timeout ×4, qcadsp8994.mbn yok, ses kartı deferred, regulatory.db yok,
  psci-cpuidle -ENODEV, l2 "bypassed regulator has no supply", i2c_qup f9967000 tx DMA yok, rmi4 vio / zpa2326 vref /
  DSI lab/ibb dummy, simple-pm-bus devlink hatası, qseecom atlandı.
- USB: kablo boot sırasında takılı değildi (smbchg-usb=0); 84 s'de takılınca configured; çıkar/tak sorunsuz.

## 12. 2026-11-10: msm DRM ile panel siyah; MDP SMMU çalışmıyor

- 0025 DTB ile art arda 5 açılış init + USB'ye ulaştı (PMIC: PS_HOLD, beklenmeyen reset yok), ama panel siyah:
  DRM "connected/enabled/DPMS On", saatler doğru, MDP/DSI kesmeleri artıyor, fakat her karede `pp done time out, lm=2`;
  bir açılışta `fd9d4000.iommu-ctx: timeout waiting for TLB SYNC` ve panel probe 5.5 s.
- #14 (ekran çalışmıştı) ile fark: GPU/CAMSS/VFE/Venus SMMU probe edilmişti. Venus SMMU, güvenli context'i olan tek
  örnek → `qcom_iommu_sec_ptbl_init()` yalnızca onun probe'unda çalışıyor.
- Deney: yalnız Venus SMMU'yu geri açan DTB → ~25 s'de ekran gitti, reset yok, USB gelmedi (init'e ulaşmadan donma).
  Yani ptbl init'li MDP yolu da güvenilir değil (#14 tek başarılı örnek; aynı yapı diğer denemelerde reset/donma).
- Karar: varsayılan girdi SAFE DT (MDSS + bütün SMMU'lar kapalı, UEFI framebuffer, fuel gauge/BT kapalı).
  msm DRM girdisi "EXPERIMENTAL" olarak duruyor. 7.3'te msm KMS IOMMU'suz çalışmıyor ("no IOMMU, bailing out").
- Açık iş: MDP SMMU (secure-id 1) — restore_sec_cfg/ptbl sırası, TLB sync, stall-on-fault; downstream 3.10 ile karşılaştır.

## 13. 2026-11-10: dış kaynak taraması — notlar (sıradaki işin girdisi)

### Downstream 3.10 (LineageOS android_kernel_sony_msm8994 cm-14.1; oneplus_msm8994 aynı taban)
- `msm8994-iommu.dtsi` MDP SMMU: `qcom,msm-smmu-v1`, reg 0xfd9cc000 **0x10000**, `qcom,iommu-secure-id = <1>`,
  ctx `mdp_0`@fd9d4000 (SID 0, NS), `mdp_1`@fd9d5000 ve `mdp_2`@fd9d6000 secure; `vdd-supply = gdsc_mdss`;
  bus oyu `mdp_ebi` 22→512 (0 / 1000 KBps); BFB: 0x2000=0x3, 0x204c=0x7fffff, 0x2060=0x1777, … (19 kayıt).
  Bizim `msm8994_mdp_bfb` 0x2000=0x3 kaydını bilerek atlıyor (referans yorumu: "impl-def bias removed").
- `msm_iommu-v1.c` attach sırası: GDSC + bus oyu + saatler → (secure ise) `scm restore_sec_cfg(sec_id, ctx)` →
  BFB → halt → ctx programla (reset ctx, TTBR0, ACTLR BPRC*, ASIDPNE, **TLBIASID + TLBSYNC**, sonra SCTLR.M) → resume →
  tekrar TLBIASID + sync. Bizim `qcom_iommu_program_ctx()` M bitini açmadan önce TLB invalidate/sync yapmıyor.
- **`mdss_mdp.c` kilit nokta:** "delay iommu attach until continous splash screen has finished handoff, as it may still
  be working with phys addr". Alternatif yol (`mdss_mdp_splash_logo.c`): splash belleğini iommu'da 1:1 (iova = phys)
  map edip öyle attach. Bizde MDP SMMU, UEFI hâlâ framebuffer'ı fiziksel adresten tararken
  (`fd901000.mdp: Adding to iommu group 0` → default DMA domain attach) açılıyor → reset/donma/TLB SYNC timeout/
  `pp done time out` tablosunun en olası açıklaması.
- Çözüm adayları: (a) `qcom_iommu`'ya `get_resv_regions` (of_iommu `iommu-addresses` 1:1) desteği + DT'de efifb
  bölgesini `iommu-addresses = <&mdss_mdp …>` ile işaretle; (b) SMMU M bitini açmadan önce MDP INTF timing engine'leri
  durdur (msm `mdp5_kms_init` bunu yapıyor ama iommu grup attach'ından SONRA); (c) 0x2000 BFB + TLBIASID/sync farkları.

### wiki.nura.eco microsoft-cityman (kayıtlı kopya: wph\webpage_nura)
- Durum 19 Eylül 2026 (Android4Lumia950/linux için): Wi-Fi & Bluetooth çalışıyor, GPU çalışıyor ama iyileştirme gerek,
  QC şarj (~9 W), pil durumu, ana hoparlör, bazı sensörler. Tablo: ekran/dokunmatik Works, USB networking Broken.
- Eski not: ath10k (PCIe QCA6174) sistemi kilitliyordu; BT ve Wi-Fi için QCA6174 rampatch/TLV blob'ları gerekiyor.
- Bizim taban = Android4Lumia950/linux master (2026-09-10, eb871dc) — daha yeni master yok. Yan dallar: `iommu`, `mdss`
  (2026-08, master'a girmiş eski hâller), `camss` (2026-08-29), `msm8994-6.x` (2024-12: "ADSP loads… MDSS hangs the
  kernel"), `djokcho` (2025-03). Wi-Fi/BT/GPU/ses için ayrıca firmware + config gerekiyor.

### Empyreal96 (rehber + packed-lumia-950xl-kernels)
- Ekran yalnız efifb (`CONFIG_DRM_MSM` kapalı, `video=efifb`); DTB'de MDSS/IOMMU yok.
- BT firmware: `qca/rampatch_00440302.bin` (sha256 998d8d1f…), `qca/nvm_00440302.bin` (8fbe6d65…). Wi-Fi: PCIe1 + ath10k.
- 5.8-rc3 config'te `CONFIG_QCOM_WDT=y`.

### Diğer
- openSUSE Hack Week: angler/bullhead ana hat; cont_splash_mem eşleme düzeltmeleri (v6.4, fe88480a6be9, c85c8a992794).
- somainline msm8994 sayfası, AOSP hardware/qcom/msm8994 (kullanıcı tarafı HAL), UniXNU-lumia950 (XNU iskeleti): kernel
  tarafı için katkı yok.

## 14. 2026-11-10: 0026 — MDP SMMU varsayılan domain'i identity (bypass)

- Downstream `mdss_iommu_ctrl()` mantığının karşılığı: MDP cihazı iommu grubuna eklendiğinde (UEFI hâlâ fiziksel
  adresten tarıyorken) çeviren DMA domain'i yerine identity domain alıyor; context bank'a dokunulmuyor. msm DRM kendi
  paging domain'ini `msm_kms_init_vm()` içinde, `mdp5_kms_init()` timing engine'leri durdurduktan sonra bağlıyor.
- Test: GRUB 2. girdi ("FULL DT, msm DRM on (TEST…)"), Image 76509d8c242f, DTB 927c2b4a77bb (0025: GPU/CAMSS/VFE/Venus kapalı).

## 15. 2026-11-10: LineageOS 23.2 (Android 16) cityman portu — giannoccarol (XDA: Lorenzogianno)

- Kernel: `giannoccarol/android_kernel_mmo_msm8994` dalı `lineage-23-cityman` (3.10.108, Android4Lumia950 MMO
  kernel + cityman; son commit 2026-11-08). Boot zinciri **LK** (Android4Lumia950 installer, Windows silinir) —
  bizim UEFI→GRUB zincirinden farklı; LK ekranı kendisi açıp downstream mdss'e devrediyor.
- Çalışanlar (ilk mesaj, 2026-11-08 build): ekran + GPU, AOD, dokunmatik, Wi-Fi (QCA6174), BT (A2DP), LTE/SMS/arama
  (CSFB), hoparlör/kulaklık/mikrofon, NFC, sensörler, deep sleep, arka/ön kamera, flaş, Venus donanım video,
  USB ADB/OTG, 3 A USB-C şarj, **UC120 PD + DisplayPort alt mode** (USB-C→HDMI 1080p60). GPS yok, SELinux permissive.
- Dosyalar: `arch/arm64/boot/dts/mmo/msm8994-mmo-cityman.dts`, `common/*.dtsi` (mdss, regulator, connectivity…),
  `panels/dsi-panel-sergej-wqhd-dualdsi-cmd.dtsi`, `drivers/misc/uc120.c`, `sound/soc/codecs/tas2552.c`,
  `Documentation/arm64/cityman-{adsp,venus,wifi,ril,credits}.txt`.
- Panel: `vddio-supply = pm8994_s4`, `qcom,dsi-panel-bias-vreg` + PMI8994 `labibb` "oled" modu (bizde lab/ibb dummy).
- **ADSP:** imzalı cityman imajı `qcadsp8994.mbn` 10 149 719 B, SHA-256 3e101699…a835e, HW_ID 009400E100450000
  (Nokia trustroot). **Bizim FFU'daki `System32\qcadsp8994.mbn` birebir aynı hash.** Angler/başka imaj PAS_INIT -2 ile reddediliyor.
- **Venus:** `qcvss8994.mbn` 1 002 800 B, SHA-256 7db914db…0182 — **FFU'daki dosya birebir aynı.**
- **Wi-Fi:** PCIe1 endpoint 0xffffffff okuyordu; ilk kurulumdan önce `GCC_PCIE_1_BCR` (GCC+0x1b40) assert/deassert →
  168c:003e. `qcom,ep-wakeirq` geri. Firmware: Android QCA6174 hw3 `qwlan30.bin` (750 788 B, f84fd11d…); FFU'daki
  Windows `qwlan30.bin` (1 068 368 B, 3b7e5f82…) **çalışmıyor**. Board kalibrasyonu `bdwlan30.bin` cihazın kendi dosyası.
- Modem: MBA 0x06e80000, metadata 0x06f80000; modem 0x07000000, peripheral 0x0ca00000 (31 MiB, ADSP ~24 + Venus ~5), ramoops 0x0f200000.

## 16. 2026-11-10: 0026 testi + 0027 (TZ secure page-table havuzu)

- 0026 (MDP SMMU identity) ile: reset/donma yok, `TLB SYNC` timeout yok, init 26.0 s, USB/SSH çalışıyor. Ama panel
  hâlâ siyah: `pp done time out, lm=2` ×2, SMMU fault mesajı yok.
- Ekranın çalıştığı tek açılış (#14) ile fark: orada Venus SMMU, MDP SMMU'dan önce probe olup
  `qcom_iommu_sec_ptbl_init()` çalıştırmıştı ("iommu sec: pgtable size: 151552"). 0025'ten beri Venus kapalı → havuz hiç
  kurulmuyor, MDP SMMU (secure-id 1) TZ tarafından havuzsuz restore ediliyor.
- 0027: havuz her TZ-yönetimli örnek için (bir kez) kuruluyor — downstream `msm_iommu_sec_ptbl_init()` gibi.
  Image 94982b91da1a, SD'ye SSH ile yazıldı (DTB aynı: 927c2b4a77bb).
- **Sonuç (0027, Image 94982b91da1a):** 2. açılışta panel msm DRM ile görüntü veriyor; `iommu sec: pgtable size: 151552`
  artık MDP SMMU'dan (fd9cc000) basılıyor, `pp done time out` 2'den 1'e indi, init 25.9 s, USB/SSH çalışıyor.
- Aynı build'in 1. açılışı ~25 s'de reset attı; init'e ulaşmadığı için log yok. Sonraki açılışta PMIC: `pon 21 poff 02`
  = PS_HOLD ile kapanma (kullanıcı tuşu değil; TZ/yazılım kaynaklı reset). Reset noktası hâlâ MDSS/MDP SMMU probe aralığı (24–25.3 s).

## 17. 2026-11-10: 0028 — OCMEM kapatıldı (yanlış iz)

- 0027'nin reset olan açılışının 120 fps videosu: son okunan satırlar `ocmem fdd00000.sram: 8 ports…` (25.439),
  `probe of fdd00000.sram … returned 0` (25.456), `l20: Bringing 0uV into 2950000uV` (25.466). İyi açılışta OCMEM'den
  hemen sonra MDP SMMU'nun `iommu sec: pgtable size` satırı geliyor.
- OCMEM'i yalnızca (kapalı) GPU kullanıyor ama probe'u VDD_GFX'i açıyor, OCMEMGX saatini ayarlıyor ve TZ
  restore_sec_cfg(OCMEM) çağırıyor. 0028 cityman'da `&ocmem`'i kapattı. **Sonuç: 2/2 açılış yine reset** → neden bu değil.
  (Kapalı kalması zararsız; GPU açılınca geri açılacak.)

## 18. 2026-11-10: 0029 — güvenli sayfa tablosu havuzu no-map bölgede (yanlış iz)

- 0028 videosu: son satırlar `mmcc … returned 0` (24.316), `qcom-iommu fd9cc000.iommu: iommu sec: pgtable size: 151552`
  (24.331), `s5: Bringing 0uV …` (24.339).
- Hipotez: msm-3.10'da `DMA_ATTR_NO_KERNEL_MAPPING` tamponu doğrusal haritadan çıkarıyor, mainline çıkarmıyor; TZ havuzu
  XPU ile kilitlediği için spekülatif CPU erişimi XPU ihlali → reset. 0029: MDP SMMU'ya 256 KiB no-map reserved-memory
  (`memory-region`). **Sonuç: yine reset**; son görünen satır bu kez interconnect probe'u (24.949) → hipotez yanlış.

## 19. 2026-11-10: modüler DRM, test modülü ve 0030/0031 (yine yanlış iz)

- Reset'in ardından pstore boş (RAM korunmuyor).
- 0029 + `CONFIG_DRM_MSM=m` (panel sürücüsü de modül): açılışta yalnız MDP SMMU probe oluyor. Video: son satır
  `iommu sec: pgtable at 0x5ff60000 (no-map reserved)` (23.834), `pgtable init done` yok → "reset `PTBL_INIT` içinde" sanıldı.
- `rescue/tools/ztsec.c` (test modülü) SAFE açılışta (sakin sistem; venus/camss/oxili GDSC'leri kapalı, MDSS açık):
  PTBL_SIZE 151552, dma havuz 0xdd100000, **PTBL_INIT ret 0, reset yok**.
- Buradan "çağrının zamanı önemli" sonucu çıkarıldı: 0030 0029'u geri aldı, 0031 havuzu downstream gibi erken
  (`qcom_scm` probe'unda, ~0.46 s) kuruyor, qcom_iommu kendi kurulumunu atlıyor. 0031 doğru ve kalıcı ama reset'i
  **çözmedi**: son görünen satır bu kez `fd_gdsc` probe'u (24.925), MDP SMMU'dan bile önce.

## 20. 2026-11-10: ekrandaki son satır reset anı değil

- Videolar kare kare sayıldı: ekran kernel zamanında 0.4 s'den 24.9 s'ye gerçek zamanda ~4 s'de geçiyor. Yani fbcon
  saniyelerce donup birikmiş günlüğü sonradan basıyor; **son görünen satır reset'in olduğu yer değil**. 17–19'daki
  "son satır" çıkarımlarının hepsi bu yüzden yanıltıcıydı.
- FULL girdisi `initcall_debug` olmadan açıldı: SD'de o açılışın günlüğü hiç yok (`dmesg-prev.txt` bir SAFE açılışı)
  → reset init'ten (SAFE'te 2.5 s) önce. Reset sabit bir saniyede değil, açılış sırasında sabit bir noktada oluyor.

## 21. 2026-11-10: 0032 — PS_HOLD warm reset seçeneği (ramoops için; işe yaramadı)

- `qcom_pon.ps_hold_warm=1`: PMIC PS_HOLD reset tipi warm (gen1 PON 0x5a/0x5b, msm-3.10 `qpnp-power-on.c`).
  FULL girdisi yalnız ramoops'a yazdı (`console=ttyMSM0`, fbcon gecikmesi olmasın diye).
- Sonuç: reset nedeni `warm 02` (warm reset gerçekten oldu) ama ramoops bölgesi (0x0f200000) yeni açılışta geçersiz
  → UEFI/DDR içeriği korumuyor. Seçenek çekirdekte duruyor, varsayılan kapalı.

## 22. 2026-11-10: kök neden — UEFI taraması sürerken MDP SMMU'da TZ `restore_sec_cfg` (0033–0035)

Yöntem değişti: SAFE açılış (hep sağlam) + FULL'a özgü DT düğümlerini **çalışma anında** tek tek açmak
(`rescue/tools/ztdt.c`, `of_changeset` ile `status = "okay"`; OF reconfig bildirimcileri cihazı oluşturuyor) ve bu
sırada `/dev/kmsg`'yi SSH ile bilgisayara akıtmak. Reset olursa varsayılan GRUB girdisi SAFE'e döner, döngü kendi kendine sürer.

| Düğüm (FULL'da açık, SAFE'te kapalı) | Sonuç |
|---|---|
| `pmic@2/fuel-gauge@4000` | bağlanmadı ("no parent regmap": SPMI çocukları reconfig ile oluşmuyor), reset yok |
| `i2c@f9963000/qpdst900@39` | apds9930 / tsl2772 bağlandı, reset yok |
| `cci@fda0c000` | i2c-qcom-cci bağlandı, reset yok |
| `iommu@fd9cc000` (MDP SMMU) | **reset, her seferinde** |

0033 (geçici adım işaretleri, her adımdan sonra 400 ms bekleme) ile akan günlük:
`PTBL_SIZE` → `PTBL_INIT ret 0` → ctx'ler oluşturuluyor → resume: saatler açık → `restore_sec_cfg(1, 0)` çağrılıyor → **reset**.
Çağrıyı yapan, ctx probe'undaki "map after parent resume" (ebeveyn SMMU runtime-resume ediliyor). O anda UEFI
framebuffer'ı MDP üzerinden hâlâ fiziksel adresten taranıyor.

Downstream `mdss_mdp.c` tam olarak bunu yazıyor: "delay iommu attach until continous splash screen has finished handoff,
as it may still be working with phys addr" — 3.10'da `restore_sec_cfg` + BFB attach anında yapılıyor (`msm_iommu-v1.c`).
0026 yalnızca domain bağlamayı ertelemişti; sürücünün kendi resume'u TZ çağrısını yine açılışta yapıyordu.

- **0034:** 8994 MDP SMMU `hw_deferred`: resume yalnız saatleri açıyor; ctx probe ebeveyni resume etmiyor (ioremap
  donanıma dokunmaz); ilk çeviren domain attach'ında (msm DRM, `mdp5_kms_init` UEFI taramasını durdurduktan sonra)
  `restore_sec_cfg` + BFB yapılıyor, sonrası normal.
- Test (SAFE + çalışma anında): MDP SMMU açıldı → reset yok. MDSS açıldı + msm DRM yüklendi → ertelenen
  `restore_sec_cfg ret 0`, `[drm] Initialized msm`, `fb0: msmdrmfb`, telefon ayakta.
- **0035:** adım işaretleri `dev_dbg`, beklemeler kaldırıldı; `DRM_MSM` ve panel tekrar yerleşik (Image 8d6fd74eaffd).
- Ders: fbcon'un son satırına bakarak reset yeri aranmaz; canlı günlük akışı (SSH) + çalışma anında DT düğümü açma çok daha hızlı.

## 23. 2026-11-10: FULL DT güvenilirliği (0034 ile)

- GRUB: FULL DT artık varsayılan, SAFE ikinci (grub.cfg 25c9a4526b7e).
- **4/4 temiz açılış** (kullanıcı açılışı + 3 safe-reboot döngüsü). Her biri ~75 s'de geri geldi (~40 s kapanış/UEFI/GRUB
  + ~35 s uptime → arada gizli bir açılış yok).
- Her açılışta: `early pgtable init done` 0.46 s, `[drm] Initialized msm` ~22 s, `msmdrmfb`, init ~22.5 s, konsol panelde,
  SMMU fault/oops yok.
- Kalanlar (reset değil): fuel gauge probe ~18 s blokluyor (2.3 → 20.4 s); DRM açılırken bir kez `pp done time out, lm=2`;
  `fb0: sys_imageblit: framebuffer is not in virtual address space`; `spmi-pmic-arb.c:786` WARNING
  (`pmic_arb_chained_irq`); Bluetooth QCA sürüm okuma zaman aşımı; `qcadsp8994.mbn` ve `regulatory.db` initramfs'te yok.
- Yamalar: 0029/0030 (ekle + geri al) ve 0033 (geçici hata ayıklama) seride tarihçe olarak duruyor; temiz seriye
  indirilirken birleştirilecek.

## 24. 2026-11-10: fuel gauge açılışı 19.6 s bekletiyordu → 0036

- Neden: probe pil ID'sini okuyup FG SRAM'ine ~10 ayrı yazma (termal katsayılar, JEITA, cutoff, sonlandırma akımları,
  resume SoC, iki maskeli yazma) yapıyordu; her biri MEMIF erişimini yeniden isteyip bir FG döngüsünü (~1.5–2 s) bekliyordu.
- Referans: downstream `qpnp-fg.c` (giannoccarol `lineage-23-cityman`; LineageOS sony aynı): `delayed_init_work()` ile
  `fg_hw_init()` probe'dan sonra iş kuyruğunda; `fg_mem_lock()/fg_mem_release()` init boyunca tek erişim izni.
- 0036: probe yalnız çevre birimlerini doğrulayıp init'i iş kuyruğuna atıyor; init tek izinle yapılıp pil sonra kaydediliyor.
- Sonuç (Image 791d69373911): `FG init took 1020 ms`, **init 22.5 s → 3.05 s**, USB/SSH 5.3 s, safe-reboot döngüsü
  75 s → 45 s. `battery`: Charging, %58, 3.89 V, 92 mA, 41.5 °C, `microsoft_bvt4d_3340mah`.

## 25. ztOS kök dosya sistemi SD'de açıldı (2026-11-10)

- Davitekk'in ztOS kurucusu `cityman` hedefiyle derlendi (Arch Linux ARM + ztMETRO + ztOSIAL, 10 GiB ext4, etiket `ZR_ARCH`), SD kartın 2. bölümüne yazıldı.
- GRUB'a tek seferlik giriş mekanizması denendi (`/ztos/grubenv` içindeki `next_entry`); FAT üzerinde yazılamadığı için çalışmadı (bölüm 27), sonunda `set default=2` kullanıldı.
- `ztos.root=LABEL=ZR_ARCH` ile initramfs `switch_root` yaptı: systemd `running`, hiç başarısız servis yok, `zerone` ile SSH çalışıyor, ztOSIAL ve ztos* servisleri ayakta.
- Ekran boştu: GPU kapalı olduğundan Mesa freedreno `msm` üzerinde GPU alamadı, wlroots renderer kuramadı, ztmetro çıktı, greetd start-limit'e takıldı. `/etc/ztos/session.env` ile `kms_swrast`/`llvmpipe` zorlandı; ztMETRO 1440x2560'ta `llvmpipe` (GLES 3.2) ile açıldı, 6 döşeme. CPU ~%300 (yazılım renderlama).
- Kalanlar: GPU, Wi-Fi, ses (ADSP), arka ışık (`no usable backlight`), BT (QCA sürüm okuma zaman aşımı), root'un bölümün tamamına genişletilmesi.

## 26. ztOS arayüzü neden yavaş: CPU/bellek ölçümleri (2026-11-10)

Ekran 3-5 fps, dokunma ~2 s gecikmeli. Ölçüm (statik aarch64 benchmark, ztMETRO durdurulmuşken, `/dev/mem` ile kayıt okuma):

| | A53 (cpu0-3) | A57 (cpu4-7) |
|---|---|---|
| etkin saat (bağımlı toplama zinciri) | ~763 MHz | ~381 MHz |
| PLL (`f9015000` / `f9016000`) | L=0x50 (1536 MHz), mux 0x80000004 (PLL0 main, /2) | aynı L, ama PLL user_ctl post-div 0x309 (A53: 0x11f) → yarı hız |
| L1 gecikmesi | 3.9 ns | 10.5 ns |
| RAM gecikmesi (64 MB zincir) | ~208 ns | ~330 ns |

- Kernelde CPU saat/cpufreq sürücüsü yok: çekirdekler UEFI'nin bıraktığı değerde. zrnperfd bunu bilip her şeyi cpu0-3'e (A53) sabitliyor (`cpuset`).
- RAM gecikmesi normalin (~100 ns) 2-3 katı: bellek yolu (BIMC/CCI) da bootloader saatinde. CCI PLL L=0x1c = 537.6 MHz.
- İlk Python benchmark'ı yanıltıcıydı: ztMETRO thread'leri `nice -15`, ölçüm `nice 0` idi.
- Denemeler: `WLR_RENDERER=pixman` çalışmıyor (ztMETRO GLES2 renderer'ı doğrudan kullanıyor). Tüm 8 çekirdeği açmak (zrnperfd durdurulup cpuset 0-7) CPU kullanımını 4→8 çekirdeğe yaydı.

Referans: MMO downstream `drivers/clk/qcom/clock-cpu-8994.c` (A53/A57/CCI alfa PLL + `a5x-lf/hf-mux`, MUX_OFFSET 0x54, hız tabloları efuse speed-bin'e bağlı, voltaj `apc0/apc1_vreg_corner`). `qcom,cpu-clock-8994` hem 8994 hem 8992 (Lumia 950) için; 950'de iki A57 eksik, DT cpu düğümleri farklı olmalı.
Sıradaki iş: mainline `clk-alpha-pll` + özel mux/div saat sağlayıcısı + `cpufreq-dt` OPP tablosu (voltaj için kesinleşmeden yüksek frekans yok), sonra BIMC/CCI oyu.

## 27. QFPROM'u /dev/mem ile okumak telefonu resetledi; grubenv tek seferlik giriş çalışmadı (2026-11-10)

- CPU hız bin'ini bulmak için ztOS içinden `/dev/mem` ile `0xfc4b80b0` ve `0xfc4bc000+0x1f0..` (QFPROM/CPR sigorta alanı) okundu. Telefon anında resetlendi (SSH "closed by remote host", uptime 0). ext4 `recovery complete`, veri kaybı yok. **Ders: QFPROM kullanıcı alanından /dev/mem ile okunmaz**; hız bin'i kernel içinden `qcom,qfprom` nvmem sürücüsüyle (TZ'ye uygun yoldan) okunmalı. (Bölüm 30 sonradan düzeltilmiş alias 0xfc4bc0b0'ın güvenli, ham alias'ların sorunlu olduğunu buldu.)
- GRUB `save_env` FAT üzerinde yazamadı: reset sonrası `next_entry=2` yerinde duruyordu ve telefon yine ztOS ile açıldı. Tek seferlik giriş mekanizması çalışmıyor; ztOS şu an her açılışta varsayılan. Rescue girdileri menüde (7 sn) elle seçilir. Düzeltme için `grub.cfg`'de `set default=2` kullanıp `grubenv` mantığı kaldırılacak.
- ztOS'ta `zerone` için `sudo` parolası artık `toor` değil (`auth could not identify password`); Welcome uygulaması çalıştırıldıktan sonra değişmiş olabilir.

## 28. A57 saati 381 → 765 MHz (düz `writel`, SCM gerekmedi) (2026-11-10)

- A57 mux yazımının önceki takılmaları TZ/XPU koruması DEĞİLDİ: "güvenli" kaynak olarak seçtiğim lfmux sel 1 aslında PLL1 (PLLA) ana çıkışı ve
  bootloader onu kapatmış; küme saatsiz kalıp PS_HOLD reseti veriyordu.
- Referans: downstream `drivers/clk/qcom/clock-cpu-8994.c`, `cpu_clock_8994_init_a57_v2()` (qcom,cpu-clock-8992 / 8994-v2 yolu) mux 0x54'e düz
  `writel_relaxed(0x6)` yazıyor = lfmux sel 3 = AUX / GPLL0 600 MHz. `sanity_check_clock_tree()` PLL USER_CTL bit[9:8] anlamını belgeliyor: 1 = /2, 3 = /4,
  hız = L × 19,2 MHz / bölen.
- Çalışan sıra (rescue FULL DT, `tools/ztcpu.c`): S11 SPMI ile 900 → 1000 mV; mux sel 3 (GPLL0); PLL f9016000 USER_CTL 0x309 → 0x109; mux sel 2 (PLL0 ana).
- Bu registerların SCM `qcom_scm_io_readl` okuması bu TZ'de -22 döner; gerekmedi.
- cpubench ile ölçüldü: cpu4/cpu5 ~765 MHz (önce ~381), L1 taraması 2574 MB/s (önce ~1290 tahmini), DRAM adımlı okuma 2690 MB/s.
- Sıradaki: aynısı A53'te (768 MHz üstü 1,18 V + mem-acc ister), CCI/BIMC oyları, sonra mainline tarzı gerçek cpufreq sürücüsü.

## 29. A57 960 MHz, CCI 538 MHz, açılışta uygulanıyor (`rescue/tools/ztboost.c`)

- A57 PLL yeniden ayarı güvenli kaynaktan (GPLL0 aux, lfmux sel 3) çalışıyor: L=90 → 861 MHz, L=100 → 956 MHz (tek çekirdek). Yük altında SoC saati kendisi
  düşürüyor (2 çekirdek 950, 3 çekirdek ~899, 4 çekirdek ilk denemede ~766, açılış ayarından sonra ~862 MHz). Bu adımlarda S11 rayı downstream CPR tavanında
  tutuldu (1000 mV, msm8992-regulator.dtsi apc1 sigorta köşesi 3).
- CCI: bootloader 268,8 MHz'de çalıştırıyor (CCI PLL L=28 = 537,6 MHz, hfmux 2 = PLL/2). hfmux 3 (PLL doğrudan) bunu iki katına çıkarıyor; güvenli kaynak
  AUX, düz `writel` çalışıyor (S8 zaten 1000 mV). Etki: L2-ıskalama/CCI gecikmesi 2 MB'de 57,7 → 42,4 ns.
- DRAM gecikmesi (~215 ns) CCI adımı ve interconnect bant genişliği oyu (mas_ampss_m0 → slv_ebi_ch0, 6,4 GB/s; MDP zaten aynı oyu veriyor) ile
  değişmiyor. BIMC/DDR saati çalışan bir RPM yolu ister: açık iş.
- `rescue/init`, açılışta `/lib/modules/ztboost.ko` yüklüyor (SAFE DT girdisindeki `ztos.noboost` ile atlanır). Doğrulandı: ztboost 2,7 s'de çalışıyor,
  cpu5 956 MHz, 4 × A57 862 MHz.
- A53 763 MHz'de kalıyor (787 MHz üstü S8'de 1180 mV köşesi ister; S8 CCI ile ortak).

## 30. CPU hız bin'i ve Microsoft'un frekans/voltaj tabloları (Windows PEP, stock DSDT)

- Hız bin'i: stock DSDT (`ffu_extract/plat/ACPI/dsdt.dsl`, `OperationRegion (FUSE, SystemMemory, 0xFC4B8000, 0x5000)` ve
  `Field QWordAcc { Offset (0x40B0) PTE, 64 }` kullanan metot) PTE'yi 0xFC4B8000 + 0x40B0 = **0xFC4BC0B0** adresinden okuyup `PTE & 7` kullanıyor:
  0 = "2.0GHz", 1 = "1.8GHz". Telefonun önceki reset'leri 0xfc4b80b0 (downstream Android sürücüsünün kullandığı ham alias) ve 0xfc4bc000 okumasından
  kaynaklandı, doğru adresten değil. 0xFC4BC0B0'ın tek `readq()` ile okunması güvenli (`rescue/tools/ztfuse.c`): PTE 10a1c71c8e4e78b0 → hız bin 0 = 2,0 GHz parça.
- Windows 10 Mobile `QCpep8994.sys` (`ffu_extract/mainos/Windows/System32/Drivers`) tam tabloları taşıyor; girdiler
  {indeks, Hz, köşe kimliği, tavan µV, nominal µV, taban µV, -1}, dosya ofsetleri 0xba138 (A53), 0xba278 ve 0xba748 (A57, iki bin), 0xba418/0xba648/0xba978 (CCI), 0xba580.
  - A53: 384 … 1555,2 MHz (600/672/768 MHz 1000 mV; 864/960 MHz nominal 1115 mV; 1248..1555 MHz nominal 1180 mV, tavan 1225 mV).
  - A57 (14 basamak, 2,0 GHz bin): 384 480 633,6 | 768 864 960 (1000 mV) | 1248 1344 1440 1536 1632 (nominal 1160 mV, tavan 1225) | 1728 1824 1958,4 MHz (1225 mV).
    Diğer tablo 1766,4 MHz'de bitiyor (1,8 GHz bin).
  - CCI: üç varyantta 921,6 / 940,8 / 787,2 MHz'e kadar; 600 MHz ve üstü 1225 mV ister.
- Bu SoC'de en yüksek köşenin tavanı bu yüzden 1225 mV (Lumia 950 / MSM8992 regülatör dtsi'sindeki 1180 mV değil).

## 31. MSM8994 için TSENS (SoC sıcaklık sensörleri)

- Yalnızca PMIC kalıbı ve pil görünüyordu; 16 TSENS sensörü (cpu0-7, gpu, pop bellek) dışarı verilmiyordu.
- MSM8974 ile aynı v0.1 TSENS IP'si, 16 sensör. Çekirdek yaması 0037: `tsens-v0_1.c` içinde MSM8994 verisi, kalibrasyon QFPROM düzeltilmiş alias
  0xfc4bc0d0'dan (üç adet 32 bit sözcük; hız bin'i ile aynı alias; doğrulandı: cal_sel 3 = iki noktalı, base0 526, base1 833, tüm ofsetler 0),
  DT düğümü tsens@fc4a9000 (TM) / fc4a8000 (SROT), IRQ SPI 184, salt okunur thermal zone'lar. Sensör numaralaması downstream
  qcom,sensor-information'dan: 7-10 = cpu0-3 (A53), 13/14/15/6 = cpu4-7 (A57), 12 = gpu, 2 = pop bellek.
- Çekirdek config: `CONFIG_NVMEM_QCOM_QFPROM=y` (bağımlılık) ve `CONFIG_QCOM_TSENS=y` (ilki kapalıyken `olddefconfig` ikincisini sessizce siliyordu). Çekirdek
  artık temiz ağaçtan derleniyor (7.3.0-rc2-sumire-g971297720829); eski "-dirty" dizgisi modül yüklemeyi (vermagic) bozardı.
- SD: yeni Image, iki DTB, rescue.cpio.gz (ztboost.ko yeni sürüme göre yeniden derlendi), grub.cfg'de 5. girdi "PREVIOUS kernel" (Image.prev + cityman.prev.dtb).
- Downstream kalibrasyonun yedekleme (redundancy) bölgesi (+0x464 …) henüz ele alınmadı.
- Sonuç (son çekirdek 7.3.0-rc2-sumire-g70b1eac4d0ff): sensörler Linux başladığında zaten çalışıyor (güvenli firmware) ama SROT TSENS_EN biti 0 okunuyor;
  mainline `init_common` bu bit yüzünden reddediyordu ("device not enabled"). Yeni `skip_en_check` özellik bayrağı probe'u düzeltti.
  Sensör 0..4 kod 0 veriyor (bağlı değil, zone yok); 5 ve 11 canlı ama sahibi bilinmiyor. Elle okuma (`rescue/tools/ztts.c`) ve sürücü uyuşuyor.
  İlk okumalar (yazılım renderlama ile tüm çekirdekler yüklüyken açılıştan hemen sonra): cpu0-3 (A53) 62-67 °C, cpu4-7 (A57) 65-77 °C, gpu 75 °C,
  PMIC kalıbı 63 °C, pil 50 °C. A57 kümesi boşta denecek yükte bile ~75 °C; saat/voltaj artışı için pay kontrolü şart.
