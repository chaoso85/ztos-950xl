# Açık işler / Open items (2026-11-08, güncellendi)

Bu liste, "sonra bakarız" diye ertelenen, varsayılan ya da yarım kalan her şeyi tutar. İş bitince `[x]` yapın, yeni ertelenen bir şey çıkınca buraya ekleyin.
Legend: **[ ]** open · **[x]** done · **(!)** blocks first boot

## 1. Cihazı açmayı engelleyenler
- [x] **USB gadget (rescue/SSH):** 2026-11-08 donanımda doğrulandı. Windows ACM (usbser) + RNDIS (rndiscmp, `MS_COMP_RNDIS`) görüyor; 192.168.77.1 <-> 192.168.77.2 ping ve anahtarlı SSH (dropbear) çalışıyor. Çözümler: TCSR 0xfd4ab000 = 1 ve LK'nın QSCRATCH değerleri (Davitekk/ztOS talkman-port `patch-dwc3-usb-sel.py`), configfs `serialnumber` (küçük harf), yeni `bcdDevice`, RNDIS önce / ACM sonra. Kalanlar:
  - [ ] LK QSCRATCH değerlerinin cityman için gerçekten gerekli olup olmadığı ayrı ayrı denenmedi (talkman değerleri kullanılıyor).
  - [ ] Windows tarafında IP elle veriliyor (`wph/usberror/phonenet.cmd`); telefonda DHCP yok.
  - [ ] Host/OTG modu kapalı (gadget-only); gerekirse dual-role + usb-role-switch.
- [x] **SD karttan açılış:** 2026-11-08 doğrulandı: stock UEFI -> rEFInd -> SD üzerindeki GRUB (`\EFI\BOOT\BOOTAA64.EFI`) -> Linux. Windows girişi eMMC'den `bootmgfw_real.efi` ile zincirleniyor. Kalan: eMMC'deki eski GRUB kurulumunun (varsa) kaldırılması; SD'de rootfs için bölüm yalnızca 3,99 GB.
- [ ] (!) **Tam eMMC yedeği:** henüz alınmadı; ilk denemeden önce şart.
- [x] **RPM regülatör takılması:** bu çekirdekte takılma yok; RPM geç ama açılıyor (~8 sn). İsteğe bağlı: RPM'i erken açtırmak.
- [ ] **PSCI eksik:** kapatma / yeniden başlatma güvenilmez.

## 1b. Tam-DT açılış günlüğünden çıkanlar (2026-11-08, ayrıntı: `research/BRINGUP_LOG.md`)
- [ ] (!) **Ekran yanmıyor** (düzeltme yazıldı, cihazda denenmedi: patch 0018 `CLK_OPS_PARENT_ENABLE`): DSI açılırken `pclk0/1`, `byte0/1_clk_src` "rcg didn't update its configuration" (DSI PHY PLL çalışmıyor); `pp done time out, lm=2`. Sıra: PHY PLL / piksel saati.
- [ ] **GPU** (0019 iommu `tbu` saatini çıkardı → GX kapalıyken SMMU yazması CPU'yu kilitledi; 0020 geri koydu, `stuck at 'off'` uyarısı beklenen; firmware initramfs'e `build_initramfs.sh ... fw/` ile giriyor, depoda yok; denenmedi): `a420_pm4.fw` / `a420_pfp.fw` initramfs'te yok (61 sn bekleme); `oxili_gfx3d_clk` stuck (GPU iommu resume, GX güç alanı); adreno `vdd`/`vddcx` ve DSI `lab_reg`/`ibb_reg` DT'de yok; GPU cooling device kaydı başarısız.
- [ ] `fb0: framebuffer is not in virtual address space` (fbcon msm fbdev'e geçerken).
- [x] `NR_CPUS` 6 -> 8 (950 XL 4xA57+4xA53) ve A53/A57 için ayrı PMU (patch 0017). Cihazda henüz denenmedi.
- [x] NFC (pn547) DT'de kapatıldı: arka kapak yokken polarite tespiti 33 sn bloklıyordu. Kapak takılınca `status = "disabled"` kaldırılacak.
- [ ] `psci-cpuidle` -ENODEV: CPU idle durumları yok (güç tüketimi).
- [x] (DT'de kapatıldı/düzeltildi, patch 0019; denenmedi) DT'de olmayan/yanlış donanım (ad7147 IRQ sözdizimi hatası da düzeltildi): si470x (5-0011), fan53555 (5-006c), ad714x (4-002c, IRQ yok), mpu6050 (2-0068), ccs sensörleri (6-0020, 6-006c, 7-0020); tsl2772 supply'ları; `i2c_qup f9967000` tx DMA kanalı.
- [ ] Gecikmeler: fuel gauge probe ~19,8 sn, RPM regülatörleri ~8,3 sn, vadc + `spurious spmi irq apid=158` WARN.
- [x] `FW_LOADER_USER_HELPER(_FALLBACK)` kapatıldı: eksik firmware artık 60 sn beklemiyor (denenmedi).
- [ ] Bluetooth ROME/QCA6390 0xfc00 timeout; ses: ADSP firmware (`qcadsp8994.mbn`) ve `regulatory.db` yok.
- [ ] `keep_bootcon` olmadan zamanlaması gerçekçi bir günlük al (efifb erken zaman damgalarını ~40 kat şişiriyor).
- [ ] "Volume was not properly unmounted" safe-reboot sonrası da çıkıyor; nedeni bulunamadı.
- [ ] `ztos-lumia.config` artık izleme (ftrace/tracefs) seçeneklerini içeriyor; kararlı sürümde kapatılmalı.

## 2. Çekirdek birleştirme (master + camss) — bilerek bırakılanlar
- [ ] `camss` dalındaki mmcc değişiklikleri alınmadı: mdss GDSC `GENPD_FLAG_NO_STAY_ON`, `oxili_gx` supply adı `vdd-gfx`, `OXILICX_AXI_CLK` binding'i. GPU/ekran sorun çıkarırsa ilk şüpheliler.
- [ ] `sync-dual-dsi` ayarı iki dalda farklı; `master`'ınki tutuldu.
- [ ] Yalnızca `Image` + DTB derlendi; modüller ve tam test yok. Cihazda açılış hiç denenmedi.
- [ ] Duke paneli için DT bağlanmadı; Sergej varsayıldı.
- [ ] İki dalın uyuşmayan çift uygulamaları (display, interconnect) temiz tek seriye indirilecek (mainline'a rebase için).

## 3. GPU (Adreno 430)
- [ ] Mesa/freedreno ile gerçek render testi yok (`kmscube`, `glmark2-es2`).
- [ ] Geçici çözümleri kök nedene indir: 32 MiB GEM başlangıcı, autosuspend yok / GX hep açık, SP/TP elle güç açma, TZ SEA veren register dump'ı, OCMEM sırası (bkz. `research/FORK_ANALYSIS.md`).
- [ ] `a420_pm4.fw` / `a420_pfp.fw` yeniden dağıtım lisansını `linux-firmware` WHENCE'inde doğrula.
- [ ] GPU olmazsa ztMETRO'nun yazılım render (simpledrm + llvmpipe) ile çalışıp çalışmadığı denenmedi.
- [ ] MDP5 dual-LM / 20nm DSI PHY yamaları fork'ta "needs review".

## 4. Kamera
- [ ] Ön ve iris kameralarının MCLK atamaları (gpio14/15) Qualcomm referansıyla çelişiyor; canlı test ya da Windows PEP tablosu ile doğrula.
- [ ] Bağlantı frekansı 342 MHz (fork) vs 336 MHz (`.dcc`).
- [ ] `.dcc` dosya formatı çözülmedi (mod tabloları, OIS register'ları).
- [ ] Sensör kimlik register'ları hiç okunmadı; IMX230 bilgisi yalnızca fork DT yorumuna dayanıyor.
- [ ] mainline `ccs` sürücüsü sensörlerle çalışmıyor ("ccs not yet").
- [ ] AF aktüatörü, OIS (Rohm BU24210 ailesi + OIS firmware), flaş (AS3638, sürücü yok); en sona iris kamerası.

## 5. Donanım verisi / analiz
- [ ] FPGA (iCE5LP2K) bit akışının bitiş sınırı sıfır dolgu tahminine dayanıyor; donanımda doğrulanmadı.
- [ ] Lattice kalibrasyon dosyasının yeri bulunamadı.
- [ ] UEFI'nin Sergej / Duke seçimi (panel ID okuma) doğrulanmadı.
- [ ] Windows PEP tabloları (regülatör ve saat eşlemeleri) incelenmedi.
- [ ] Downstream MDSS dtsi dosyaları çekilmedi.
- [ ] Telefondan canlı veri toplama (root uygulama) fikri bırakıldı; Linux açılınca kendisi okuyabilir.

## 6. Hiç başlanmayanlar
- [ ] Wi-Fi: pcie1 / RP1 DT'ye bağlı değil (ath10k + IRQ).
- [ ] Modem / SIM (ADSP yükleniyor, modem yüklenmiyor).
- [ ] Sensörler, titreşim, NFC; ses (WCD9330 / TAS2552 / ADSP) ve şarj (SMBCHG / FG) temizliği.
- [ ] ztOS tarafı: `ztconflo` için `uefi-esp` flash_method, ztOSIAL manifesti (`/vendor/ztos/manifest.toml`), `ztos-devices` kaydı (`device.toml`), rootfs / GSI imajı.
- [ ] Kurulumda firmware'i telefonun kendisinden çıkaran betik (hiçbir üretici ikilisi depoya girmeyecek).
- [ ] LumiaWOA tarzı genel yüklenebilir imaj / yükleyici.

## 7. Düzen ve gizlilik
- [x] GitHub'da yalnızca `chaoso85` ve GitHub noreply adresi kullanılır; `Co-Authored-By` yok, commit imzalama kapalı.
- [ ] Yeni repoda commit sayfasında görünen yazar adı kontrol edilecek (yalnızca `chaoso85` olmalı).
- [ ] Diğer repolardaki eski commit yazarları gözden geçirilecek (bu oturumda başka repo bağlanmadı).
