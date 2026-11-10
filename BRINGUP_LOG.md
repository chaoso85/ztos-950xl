
## 25. ztOS kök dosya sistemi SD'de açıldı (2026-11-10)

- Davitekk'in ztOS kurucusu `cityman` hedefiyle derlendi (Arch Linux ARM + ztMETRO + ztOSIAL, 10 GiB ext4, etiket `ZR_ARCH`), SD kartın 2. bölümüne yazıldı.
- GRUB'a tek seferlik giriş mekanizması eklendi (`/ztos/grubenv` içindeki `next_entry`, GRUB onu açılıştan önce siler). Takılırsa sonraki açılış 0. girdiye (rescue) döner.
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
