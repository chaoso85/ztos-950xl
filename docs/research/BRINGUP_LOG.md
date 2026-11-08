# Cihaz üstü bring-up günlüğü (2026-11-07 / 2026-11-08)

Bu belge, çekirdeğin ilk kez gerçek telefonda (Lumia 950 XL, cityman) çalıştırılmasından çıkan **doğrulanmış** bulguları toplar. Önceki araştırma notları (PORTING_GAPS, FINDINGS_feasibility) yalnızca çevrimdışı analizdi; burada cihazda ölçülenler var.

## 1. Açılış zinciri (çalışıyor)

```
stock UEFI (eMMC) -> rEFInd -> GRUB arm64-efi (SD: \EFI\BOOT\BOOTAA64.EFI) -> Linux 7.3-rc2 (Android4Lumia950 tabanı) + initramfs
```

- Windows (eMMC) değiştirilmedi; GRUB menüsünde "Windows (eMMC)" girişi `bootmgfw_real.efi`'ye zincirler.
- GRUB'da `loadenv/save_env` modülleri yok, yani tek seferlik açılış seçimi yapılamıyor; giriş 7 sn'lik menüden elle seçilir (varsayılan 0 = SAFE).
- Girişler (`rescue/grub.cfg`): SAFE (DWC3), FULL DT (ekran/GPU açık, `keep_bootcon`), SAFE-ChipIdea, Windows.

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

SD düzeni: `\ztos\{Image, rescue.cpio.gz, grub.cfg, msm8994-msft-lumia-octagon-cityman{,-safe,-safe-ci}.dtb, logs}`;
`update_sd.ps1` bunları yazar, hash + grub.cfg referanslarını doğrular, eski dosyaları siler.
