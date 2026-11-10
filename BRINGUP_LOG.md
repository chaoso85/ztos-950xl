
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

## 27. QFPROM'u /dev/mem ile okumak telefonu resetledi; grubenv tek seferlik giriş çalışmadı (2026-11-10)

- CPU hız bin'ini bulmak için ztOS içinden `/dev/mem` ile `0xfc4b80b0` ve `0xfc4bc000+0x1f0..` (QFPROM/CPR sigorta alanı) okundu. Telefon anında resetlendi (SSH "closed by remote host", uptime 0). ext4 `recovery complete`, veri kaybı yok. **Ders: QFPROM kullanıcı alanından /dev/mem ile okunmaz**; hız bin'i kernel içinden `qcom,qfprom` nvmem sürücüsüyle (TZ'ye uygun yoldan) okunmalı.
- GRUB `save_env` FAT üzerinde yazamadı: reset sonrası `next_entry=2` yerinde duruyordu ve telefon yine ztOS ile açıldı. Tek seferlik giriş mekanizması çalışmıyor; ztOS şu an her açılışta varsayılan. Rescue girdileri menüde (7 sn) elle seçilir. Düzeltme için `grub.cfg`'de `set default=2` kullanıp `grubenv` mantığı kaldırılacak.
- ztOS'ta `zerone` için `sudo` parolası artık `toor` değil (`auth could not identify password`); Welcome uygulaması çalıştırıldıktan sonra değişmiş olabilir.

## 28. A57 clock raised 381 -> 765 MHz (plain writel, no SCM)

- Earlier hangs of the A57 mux write were NOT a TZ/XPU protection: my "safe" source was lfmux sel 1,
  which is PLL1 (PLLA) main and is switched off by the bootloader, so the cluster lost its clock (PS_HOLD reset).
- Reference: downstream `drivers/clk/qcom/clock-cpu-8994.c`, `cpu_clock_8994_init_a57_v2()` (the qcom,cpu-clock-8992 / 8994-v2 path)
  writes mux 0x54 with plain `writel_relaxed(0x6)` = lfmux sel 3 = AUX / GPLL0 600 MHz. `sanity_check_clock_tree()` documents
  PLL USER_CTL bits[9:8]: 1 = div 2, 3 = div 4, rate = L * 19.2 MHz / div.
- Working sequence (rescue FULL DT, tools/ztcpu.c): S11 900 -> 1000 mV over SPMI; mux sel 3 (GPLL0); PLL f9016000 USER_CTL 0x309 -> 0x109; mux sel 2 (PLL0 main).
- SCM `qcom_scm_io_readl` of these registers returns -22 on this TZ; not needed.
- Measured with cpubench: cpu4/cpu5 ~765 MHz (was ~381), L1 sweep 2574 MB/s (was ~1290 est.), DRAM stride 2690 MB/s.
- Next: the same on A53 (above 768 MHz needs 1.18 V + mem-acc), CCI/BIMC votes, then a real mainline-style cpufreq driver.

## 29. A57 960 MHz, CCI 538 MHz, applied at boot (rescue/tools/ztboost.c)

- A57 PLL retune works from the safe source (GPLL0 aux, lfmux sel 3): L=90 -> 861 MHz, L=100 -> 956 MHz (single core). The SoC lowers the clock itself
  with more loaded A57 cores (2 cores 950, 3 cores ~899, 4 cores ~766 in the first run, ~862 after the boot-time setup). Rail S11 held at the
  downstream CPR ceiling for these steps (1000 mV, msm8992-regulator.dtsi apc1 fuse corner 3).
- CCI: bootloader runs it at 268.8 MHz (CCI PLL L=28 = 537.6 MHz, hfmux 2 = PLL/2). hfmux 3 (PLL direct) doubles it, through AUX as safe source,
  plain writel works (S8 is already 1000 mV). Effect: L2-miss/CCI latency 57.7 -> 42.4 ns at 2 MB.
- DRAM latency is unchanged (~215 ns) by the CCI step and by an interconnect bandwidth vote (mas_ampss_m0 -> slv_ebi_ch0, 6.4 GB/s; the MDP already votes
  that). BIMC/DDR clock needs a working RPM path: open item.
- SCM io_readl of these registers returns -22; not needed.
- rescue/init loads /lib/modules/ztboost.ko at boot (skipped with ztos.noboost, set on the SAFE DT entry). Verified: ztboost runs at 2.7 s,
  cpu5 956 MHz, 4 x A57 862 MHz.
- A53 stays at 763 MHz (above 787 MHz needs the 1180 mV corner on S8, shared with the CCI).

## 30. CPU speed bin and Microsoft's frequency/voltage tables (Windows PEP, stock DSDT)

- Speed bin: the stock DSDT (ffu_extract/plat/ACPI/dsdt.dsl, method with `OperationRegion (FUSE, SystemMemory, 0xFC4B8000, 0x5000)`,
  `Field QWordAcc { Offset (0x40B0) PTE, 64 }`) reads PTE at 0xFC4B8000 + 0x40B0 = **0xFC4BC0B0** and uses `PTE & 7`: 0 = "2.0GHz", 1 = "1.8GHz".
  The earlier phone resets came from reading 0xfc4b80b0 (raw alias, as the downstream Android driver does) and 0xfc4bc000 - not from the
  right address. One readq() of 0xFC4BC0B0 (rescue/tools/ztfuse.c) is safe: PTE 10a1c71c8e4e78b0 -> speed bin 0 = the 2.0 GHz part.
- Windows 10 Mobile's QCpep8994.sys (ffu_extract/mainos/Windows/System32/Drivers) carries the full tables, entries
  {index, Hz, corner id, ceiling uV, nominal uV, floor uV, -1} at file offsets 0xba138 (A53), 0xba278 and 0xba748 (A57, two bins), 0xba418/0xba648/0xba978 (CCI), 0xba580.
  - A53: 384 .. 1555.2 MHz (600/672/768 MHz at 1000 mV; 864/960 MHz nominal 1115 mV; 1248..1555 MHz nominal 1180 mV, ceiling 1225 mV).
  - A57 (14 steps, the 2.0 GHz bin): 384 480 633.6 | 768 864 960 (1000 mV) | 1248 1344 1440 1536 1632 (nominal 1160 mV, ceiling 1225) | 1728 1824 1958.4 MHz (1225 mV).
    The other table ends at 1766.4 MHz (the 1.8 GHz bin).
  - CCI: up to 921.6 / 940.8 / 787.2 MHz in three variants; 600 MHz and up needs 1225 mV.
- Ceiling for the highest corner on this SoC is therefore 1225 mV (not 1180 mV as in the Lumia 950 / MSM8992 regulator dtsi).

## 31. TSENS (SoC temperature sensors) for MSM8994

- Only the PMIC die and the battery were visible; the 16 TSENS sensors (cpu0-7, gpu, pop memory) were not exposed.
- Same v0.1 TSENS IP as MSM8974, 16 sensors. Kernel patch 0037: MSM8994 data in tsens-v0_1.c, calibration read from QFPROM corrected
  alias 0xfc4bc0d0 (three 32-bit words, same alias as the speed bin; verified: cal_sel 3 = two-point, base0 526, base1 833, all offsets 0),
  DT node tsens@fc4a9000 (TM) / fc4a8000 (SROT), IRQ SPI 184, read-only thermal zones. Sensor numbering from the downstream
  qcom,sensor-information: 7-10 = cpu0-3 (A53), 13/14/15/6 = cpu4-7 (A57), 12 = gpu, 2 = pop memory.
- Kernel config: CONFIG_NVMEM_QCOM_QFPROM=y (dependency) and CONFIG_QCOM_TSENS=y. The kernel now builds from a clean tree
  (7.3.0-rc2-sumire-g971297720829); the old "-dirty" string would have broken module loading (vermagic).
- SD: new Image, both DTBs, rescue.cpio.gz (ztboost.ko rebuilt for the new release), grub.cfg with entry 4 "PREVIOUS kernel" (Image.prev + cityman.prev.dtb).
- Redundancy region (+0x464 ...) of the downstream calibration is not handled yet.
