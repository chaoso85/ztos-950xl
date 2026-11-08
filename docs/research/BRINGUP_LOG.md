# Cihaz üstü bring-up günlüğü (2026-11-07 / 2026-11-08)

Bu belge, çekirdeğin ilk kez gerçek telefonda (Lumia 950 XL, cityman) çalıştırılmasından çıkan **doğrulanmış** bulguları toplar. Önceki araştırma notları (PORTING_GAPS, FINDINGS_feasibility) yalnızca çevrimdışı analizdi; burada cihazda ölçülenler var.

## 1. Açılış zinciri (çalışıyor)

```
stock UEFI (eMMC) -> rEFInd -> GRUB arm64-efi (SD: \EFI\BOOT\BOOTAA64.EFI) -> Linux 7.3-rc2 (Android4Lumia950 tabanı) + initramfs
```

- Windows (eMMC) değiştirilmedi; GRUB menüsünde "Windows (eMMC)" girişi `bootmgfw_real.efi`'ye zincirler.
- GRUB'da `loadenv/save_env` modülleri yok, yani tek seferlik açılış seçimi yapılamıyor; giriş 7 sn'lik menüden elle seçilir (varsayılan 0 = FULL DT).
- Girişler (`rescue/grub.cfg`, 2026-11-08 sadeleştirildi): FULL DT (ekran/GPU açık), FULL DT msm DRM kapalı (ekran sorununu ayırmak için), Windows. SAFE/SAFE-ChipIdea girdileri ve DTB'leri SD'den kaldırıldı (DTS'ler kaynak ağacında duruyor).

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

## 5. Sırada

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
