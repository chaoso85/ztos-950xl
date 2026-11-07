# Açık işler / Open items (2026-11-07)

Bu liste, "sonra bakarız" diye ertelenen, varsayılan ya da yarım kalan her şeyi tutar. İş bitince `[x]` yapın, yeni ertelenen bir şey çıkınca buraya ekleyin.
Legend: **[ ]** open · **[x]** done · **(!)** blocks first boot

## 1. Cihazı açmayı engelleyenler
- [ ] (!) **USB gadget (rescue/SSH):** DT tarafı hazır, çekirdek derleniyor (`usb3`, UC120, SMBCHG açık; DWC3 yalnızca gadget; configfs ACM/ECM/NCM/RNDIS). **Donanımda doğrulanmadı.** Kalanlar:
  - [ ] Gerçek HS/SS PHY sürücüsü yok: şimdilik UEFI'nin ilklediği PHY'ye güveniliyor, `usb-nop-xceiv` yalnızca l24'ü açık tutuyor (l6 always-on). Çalışmazsa downstream `qcom,usb-hsphy` init değerleri (0x00D191A4 / 0x00D1D9A4) ile PHY sürücüsü yazılacak.
  - [ ] UC120 bit akışı runtime'da `request_firmware` ile isteniyor; telefondan çıkarma betiği henüz yok.
  - [ ] initramfs içinde gadget (configfs) + SSH/ACM kurtarma betiği yazılacak.
  - [ ] Host/OTG modu kapalı (gadget-only); gerekirse dual-role + usb-role-switch.
- [ ] (!) **SD karttan açılış:** UEFI'nin SD'den okuyabildiği doğrulanmadı. Bilinen çalışan yol: eMMC ESP üzerinde GRUB, `\EFI\Microsoft\Boot\bootmgfw.efi` olarak kopyalı (fallback yolu çalışmıyor).
- [ ] (!) **Tam eMMC yedeği:** henüz alınmadı; ilk denemeden önce şart.
- [ ] **RPM regülatör takılması:** topluluk mainline 6.16'da framebuffer'dan sonra bu noktada takıldığını bildirdi; fork'ta düzelip düzelmediği bilinmiyor.
- [ ] **PSCI eksik:** kapatma / yeniden başlatma güvenilmez.

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
