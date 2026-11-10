
## 25. ztOS kök dosya sistemi SD'de açıldı (2026-11-10)

- Davitekk'in ztOS kurucusu `cityman` hedefiyle derlendi (Arch Linux ARM + ztMETRO + ztOSIAL, 10 GiB ext4, etiket `ZR_ARCH`), SD kartın 2. bölümüne yazıldı.
- GRUB'a tek seferlik giriş mekanizması eklendi (`/ztos/grubenv` içindeki `next_entry`, GRUB onu açılıştan önce siler). Takılırsa sonraki açılış 0. girdiye (rescue) döner.
- `ztos.root=LABEL=ZR_ARCH` ile initramfs `switch_root` yaptı: systemd `running`, hiç başarısız servis yok, `zerone` ile SSH çalışıyor, ztOSIAL ve ztos* servisleri ayakta.
- Ekran boştu: GPU kapalı olduğundan Mesa freedreno `msm` üzerinde GPU alamadı, wlroots renderer kuramadı, ztmetro çıktı, greetd start-limit'e takıldı. `/etc/ztos/session.env` ile `kms_swrast`/`llvmpipe` zorlandı; ztMETRO 1440x2560'ta `llvmpipe` (GLES 3.2) ile açıldı, 6 döşeme. CPU ~%300 (yazılım renderlama).
- Kalanlar: GPU, Wi-Fi, ses (ADSP), arka ışık (`no usable backlight`), BT (QCA sürüm okuma zaman aşımı), root'un bölümün tamamına genişletilmesi.
