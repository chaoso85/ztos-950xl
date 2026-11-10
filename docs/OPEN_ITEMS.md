# Açık işler / Open items (güncelleme: 2026-11-10, akşam)

Bu liste, "sonra bakarız" diye ertelenen, varsayılan ya da yarım kalan her şeyi tutar. İş bitince `[x]` yapın, yeni ertelenen bir şey çıkınca buraya ekleyin.
Legend: **[ ]** open · **[x]** done · **(!)** blocks first boot

## 1. Cihazı açmayı engelleyenler
- [x] **USB gadget (rescue/SSH):** 2026-11-08 donanımda doğrulandı. Windows ACM (usbser) + RNDIS (rndiscmp, `MS_COMP_RNDIS`) görüyor; 192.168.77.1 <-> 192.168.77.2 ping ve anahtarlı SSH (dropbear) çalışıyor. Çözümler: TCSR 0xfd4ab000 = 1 ve LK'nın QSCRATCH değerleri (Davitekk/ztOS talkman-port `patch-dwc3-usb-sel.py`), configfs `serialnumber` (küçük harf), yeni `bcdDevice`, RNDIS önce / ACM sonra. Kalanlar:
  - [ ] LK QSCRATCH değerlerinin cityman için gerçekten gerekli olup olmadığı ayrı ayrı denenmedi (talkman değerleri kullanılıyor).
  - [ ] Windows tarafında IP elle veriliyor (`wph/usberror/phonenet.cmd`); telefonda DHCP yok.
  - [ ] Host/OTG modu kapalı (gadget-only); gerekirse dual-role + usb-role-switch.
- [x] **SD karttan açılış:** 2026-11-08 doğrulandı: stock UEFI -> rEFInd -> SD üzerindeki GRUB (`\EFI\BOOT\BOOTAA64.EFI`) -> Linux. Windows girişi eMMC'den `bootmgfw_real.efi` ile zincirleniyor. Kalan: eMMC'deki eski GRUB kurulumunun (varsa) kaldırılması. (SD'de artık ztOS kökü var: 2. bölüm 25,7 GiB, içindeki ext4 10 GiB; bkz. 1d.)
- [ ] (!) **Tam eMMC yedeği:** henüz alınmadı; ilk denemeden önce şart.
- [x] **RPM regülatör takılması:** bu çekirdekte takılma yok; RPM geç ama açılıyor (~8 sn). İsteğe bağlı: RPM'i erken açtırmak.
- [ ] **PSCI eksik:** kapatma / yeniden başlatma güvenilmez.

## 1b. Tam-DT açılış günlüğünden çıkanlar (2026-11-08, ayrıntı: `research/BRINGUP_LOG.md`)
- [x] **Ekran:** 2026-11-10 çalışıyor (msm DRM, Sergej, 1440x2560, konsol panelde, 4/4 temiz açılış). Yollar: 0018/0021 (DSI saatleri), 0026 (MDP identity domain), 0027/0031 (TZ güvenli sayfa tablosu havuzu), **0034** (MDP SMMU TZ `restore_sec_cfg` DRM attach'ına kadar ertelendi; kök neden `research/MDP_SMMU_RESET.md`).
- [ ] **GPU** (2026-11-10: GPU, GPU SMMU ve OCMEM DT'de kapalı — 0024/0028. Açılırken MDP'deki kural uygulanacak: rail/saat açılmadan SMMU'ya TZ/BFB/context yazması yok.) (Eski not: 0019 iommu `tbu` saatini çıkardı → GX kapalıyken SMMU yazması CPU'yu kilitledi; 0020 geri koydu, `stuck at 'off'` uyarısı beklenen; firmware initramfs'e `build_initramfs.sh ... fw/` ile giriyor, depoda yok; denenmedi): `a420_pm4.fw` / `a420_pfp.fw` initramfs'te yok (61 sn bekleme); `oxili_gfx3d_clk` stuck (GPU iommu resume, GX güç alanı); adreno `vdd`/`vddcx` ve DSI `lab_reg`/`ibb_reg` DT'de yok; GPU cooling device kaydı başarısız.
- [ ] `fb0: framebuffer is not in virtual address space` (fbcon msm fbdev'e geçerken).
- [x] `NR_CPUS` 6 -> 8 (950 XL 4xA57+4xA53) ve A53/A57 için ayrı PMU (patch 0017). Cihazda doğrulandı: 8 çekirdek açılıyor.
- [x] NFC (pn547) DT'de kapatıldı: arka kapak yokken polarite tespiti 33 sn bloklıyordu. Kapak takılınca `status = "disabled"` kaldırılacak.
- [ ] `psci-cpuidle` -ENODEV: CPU idle durumları yok (güç tüketimi).
- [x] (DT'de kapatıldı/düzeltildi, patch 0019; denenmedi) DT'de olmayan/yanlış donanım (ad7147 IRQ sözdizimi hatası da düzeltildi): si470x (5-0011), fan53555 (5-006c), ad714x (4-002c, IRQ yok), mpu6050 (2-0068), ccs sensörleri (6-0020, 6-006c, 7-0020); tsl2772 supply'ları; `i2c_qup f9967000` tx DMA kanalı.
- [x] Fuel gauge gecikmesi: 0036 ile çözüldü (init iş kuyruğunda, tek MEMIF izni; init 3.05 s).
- [ ] Her açılışta: `spmi-pmic-arb.c:786` WARNING (`pmic_arb_chained_irq`, eski adıyla spurious spmi irq apid=158) her açılışta.
- [x] `FW_LOADER_USER_HELPER(_FALLBACK)` kapatıldı: eksik firmware artık 60 sn beklemiyor (denenmedi).
- [ ] Bluetooth ROME/QCA6390 0xfc00 timeout; ses: ADSP firmware (`qcadsp8994.mbn`) ve `regulatory.db` yok.
- [x] `keep_bootcon` olmadan zamanlaması gerçekçi günlük alındı (2026-11-09'dan beri her açılış).
- [ ] "Volume was not properly unmounted" safe-reboot sonrası da çıkıyor; nedeni bulunamadı.
- [ ] `ztos-lumia.config` artık izleme (ftrace/tracefs) seçeneklerini içeriyor; kararlı sürümde kapatılmalı.

## 1c. 2026-11-10 reset avından kalanlar (ayrıntı: `research/BRINGUP_LOG.md` 17–23)
- [ ] DRM açılırken bir kez `pp done time out, lm=2`; panel beslemeleri downstream'den farklı (vddio = pm8994_s4, PMI8994 LAB/IBB "oled" modu; bizde l12 + dummy lab/ibb).
- [ ] MDP SMMU'da downstream'e göre kalan farklar: BFB 0x2000 = 0x3 (MICRO_MMU_CTRL) yazılmıyor; context programlanırken SCTLR.M'den önce TLBIASID + TLBSYNC yok; `mdp_ebi` bus oyu yok.
- [ ] GPU, Venus ve VFE SMMU'ları açılırken 0034'teki erteleme yaklaşımı (blok güçlenip gerçek domain bağlanana kadar TZ/BFB/context yok).
- [ ] OCMEM cityman'da kapalı (0028); GPU açılınca geri açılacak.
- [ ] Yama serisini temizle: 0029/0030 (ekle + geri al) ve 0033 (geçici hata ayıklama) birleştirilecek; `MDP_STEP` işaretleri (`dev_dbg`) kararlı sürümde kaldırılabilir.
- [ ] `qcom_pon.ps_hold_warm` seçeneği (0032) duruyor, varsayılan kapalı; ramoops UEFI warm reset'te de korunmuyor. Kalıcı çökme günlüğü için başka yol (ör. SD'ye erken kmsg yazımı) gerekirse düşünülecek.
- [ ] SD'de deney artıkları: `ztos/ko/`, `ztos/loaddrm.sh`, `cityman.prev.dtb`, `msm8994-msft-lumia-octagon-cityman.prev.dtb` (zararsız; temizlenecek).
- [ ] ADSP: imzalı `qcadsp8994.mbn` (FFU'dakiyle aynı hash) kurulumda telefondan alınıp initramfs/firmware yoluna konacak; ses kartı ondan sonra.
- [ ] Wi-Fi: PCIe1 + `GCC_PCIE_1_BCR` reset + Android QCA6174 `qwlan30.bin` / cihazın `bdwlan30.bin`'i. BT: `rampatch_00440302.bin` / `nvm_00440302.bin` ve güç sırası.

## 1d. 2026-11-10 akşam: ztOS kökü, CPU saatleri, TSENS (ayrıntı: `research/BRINGUP_LOG.md` 25–31)
- [x] ztOS kökü SD'nin 2. bölümünden açılıyor (`ztos.root=LABEL=ZR_ARCH`); yazılım renderlama (llvmpipe) `/etc/ztos/session.env` ile zorlanmış.
- [x] A57 960 MHz + CCI 538 MHz açılışta (`ztboost.ko`), S11 1,0 V; hız bin'i 0 (2,0 GHz parça); Windows PEP tabloları çıkarıldı; TSENS thermal zone'ları (0037).
- [ ] **A57'yi kademeli yükselt** (1248 → 1536 MHz, tavan 1225 mV) TSENS sıcaklığı izlenerek; A57 şimdiden boşta denecek yükte ~75 °C: sıcaklık payı ve soğutma/azaltma politikası (hot trip 115 °C DT'de var, eylem yok).
- [ ] **A53 787 MHz üstü:** S8'de 1180–1225 mV köşesi (CCI ile ortak) + mem-acc.
- [ ] **DDR/BIMC saati:** DRAM gecikmesi ~215 ns (normal ~100); RPM yolu ya da oy etkisiz kaldı. İnterconnect oyu denendi, etkisi yok.
- [ ] **Gerçek cpufreq sürücüsü:** `clk-alpha-pll` + mux/div sağlayıcı + OPP tablosu (`ztboost` geçici çözüm); SoC'nin yük altında A57 saatini kendisinin düşürmesi (4 çekirdekte ~862 MHz) incelenecek.
- [ ] TSENS: yedek (redundancy) kalibrasyon bölgesi (+0x464…), sensör 5 ve 11'in sahibi, 0–4 bağlı değil; hot trip için gerçek eylem.
- [ ] Kalıcı 8 çekirdekli cpuset: zrnperfd her şeyi A53'e (cpu0-3) sabitliyor; A57 hızlandırması bundan faydalanmıyor.
- [ ] Çekirdek modülleri rootfs `/lib/modules` içine; kök dosya sistemi bölüm boyutuna genişletilecek (10 GiB / 25,7 GiB).
- [ ] ztOS: `no usable backlight` (arka ışık), BT, ses, Wi-Fi; `zerone` sudo parolası ztOS'ta değişmiş görünüyor (Welcome uygulaması sonrası).
- [x] GRUB tek seferlik giriş (`grubenv`) FAT'a yazamadığı için çalışmadı → `set default=2`; rescue girdileri menüden elle seçilir.
- [ ] /dev/mem ile QFPROM okurken dikkat: yalnızca düzeltilmiş alias'lar (0xfc4bc0b0, 0xfc4bc0d0…) güvenli; ham alias'lar (0xfc4b80b0, 0xfc4bc000) telefonu resetliyor.

## 2. Çekirdek birleştirme (master + camss) — bilerek bırakılanlar
- [ ] `camss` dalındaki mmcc değişiklikleri alınmadı: mdss GDSC `GENPD_FLAG_NO_STAY_ON`, `oxili_gx` supply adı `vdd-gfx`, `OXILICX_AXI_CLK` binding'i. GPU/ekran sorun çıkarırsa ilk şüpheliler.
- [ ] `sync-dual-dsi` ayarı iki dalda farklı; `master`'ınki tutuldu.
- [ ] Yalnızca `Image` + DTB (ve elle derlenen birkaç modül) üretildi; tam modül seti ve kapsamlı test yok. Cihazda açılış çalışıyor.
- [ ] Duke paneli için DT bağlanmadı; Sergej varsayıldı.
- [ ] İki dalın uyuşmayan çift uygulamaları (display, interconnect) temiz tek seriye indirilecek (mainline'a rebase için).

## 3. GPU (Adreno 430)
- [ ] Mesa/freedreno ile gerçek render testi yok (`kmscube`, `glmark2-es2`).
- [ ] Geçici çözümleri kök nedene indir: 32 MiB GEM başlangıcı, autosuspend yok / GX hep açık, SP/TP elle güç açma, TZ SEA veren register dump'ı, OCMEM sırası (bkz. `research/FORK_ANALYSIS.md`).
- [ ] `a420_pm4.fw` / `a420_pfp.fw` yeniden dağıtım lisansını `linux-firmware` WHENCE'inde doğrula.
- [x] ztMETRO yazılım render (llvmpipe, `kms_swrast`) ile çalışıyor ama 3–9 fps; GPU şart.
- [ ] **Sıradaki iş:** GPU güç sırası (rail → saat → SMMU; gerçek domain bağlanana kadar TZ/BFB/context yok), `a420_pm4/pfp.fw` initramfs'e, ardından `kmscube` / `glmark2-es2`.
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
- [x] Windows PEP tabloları: CPU/CCI frekans-voltaj tabloları çıkarıldı (`research/BRINGUP_LOG.md` 30). Kalan: GPU/bellek/regülatör eşlemeleri.
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
- [x] Araştırma sayfalarının İngilizce sürümleri `docs/research/en/` altında (2026-11-10).
