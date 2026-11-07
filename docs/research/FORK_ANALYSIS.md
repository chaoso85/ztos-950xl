# Android4Lumia950/linux cati analizi (2026-11-07)

Kaynak: github.com/Android4Lumia950/linux, dallar: master (2026-09-10), camss (2026-08-29), mdss, iommu, msm8994-6.x. Yazar: sonic011gamer (+ Dmitry Baryshkov'dan alinan qcom_iommu serisi).
Ozel commit sayisi (master, 2026-08-20 sonrasi): 43; dokunulan dosya: 71. Tum liste: ztos-lumia/fork_master_commits.txt (bulut calisma alani).
ONEMLI: Cityman ekran paneli (Sergej, panel-microsoft-sergej.c) ve CAMSS 8x94 `camss` dalinda; master'da talkman paneli (AMS567DJ06) var. Tam taban = master + camss birlesimi (henuz birlestirilmemis).

## Alt sistem tablosu
| Alt sistem | Fork'ta ne var | Durum etiketi (commit mesajindan) |
|---|---|---|
| GPU (A430) | a4xx_catalog 0x04030000/02, a4xx_gpu SP/TP power-on, OCMEM gecikmeli alloc, msm8994-oxili on-guc helper, GEM 32MiB baslangic, autosuspend yok, rpmpd s2b corner, ubwc no_config, ocmem 8994 quirk, mmcc GPU workaround | [DIRTY], [CLEAN UP LATER], [FIXUP LATER] |
| Ekran (MDP5/DSI) | 20nm DSI PHY msm8994/92 destegi, dsi_cfg msm8992, mdp5 dual LM/ping-pong + cmd encoder + ctl degisiklikleri, panel-microsoft-sergej (camss dali), Samsung AMS567DJ06 (talkman) | [NEED REWRITE/REVIEW?], [NEEDS CLEAN IMPL], WIP |
| IOMMU/interconnect | qcom_iommu 8974/8994 (Baryshkov serisi + MDP/GPU iommu), msm8994 interconnect surucusu + binding | [decommonize later] |
| Kamera | camss 8x94 (csiphy 2ph_1_0, csid 4_1, ispif, vfe), cityman DT: cci + 3 "nokia,smia" sensor, pinctrl | "ccs not yet" |
| USB-C / sarj | Lattice UC120 CC PHY, PMI8994/96 SMBCHG, PMI8994 fuel gauge | [SPLIT LATER], [NEEDS REVIEW] |
| Ses / modem | WCD9330 codec, TAS2552, q6 ADSP remoteproc 8994, slimbus, SoC machine driver, SCM PIL shared heap | [CLEAN UP LATER] |
| Diger | talkman kamera dugmesiyle hizli reboot [DNM], mmo_defconfig [DNM], memory map for LK [REVERT LATER] | - |

## Dogrulama: Sergej panel init dizisi (3 bagimsiz kaynak ayni)
fork panel-microsoft-sergej.c == downstream mmo dtsi == UEFI display DXE (benim FFU cikarimim):
11 (sleep out, 5 ms) -> F0 5A 5A -> F2 63 -> BD 11 01 02 16 02 16 -> F0 A5 A5 (23 ms) -> 53 20 -> 51 80 -> 35 00 -> 29. Kapatma: 28 (33 ms) -> 10 (120 ms).
Not: UEFI'de Sergej/Duke secimi (ID okuma) hala dogrulanmadi; 950 XL'de Sergej varsayimi downstream cityman dts'ine dayaniyor.

## GPU hack'leri -> kok neden arastirma listesi ("kusursuz port" icin)
1. GEM'in 16 MiB'de izin hatasi (memptrs): olasi neden SMMU context bank / secure aperture catismasi veya A430 VA ayrilmis bolge. Bak: downstream kgsl iommu "secure" ve global pagetable aralikgi, qcom_iommu BFP/prefetch ayarlari.
2. autosuspend'te SoC SEA: SCM(18) (restore_sec_cfg) GX kapaliyken -EINVAL. Dogru cozum: GX/CX GDSC + clock sirasini runtime PM callback'ine (resume) koymak, "hep acik" hack'i kaldirmak. Bak: downstream kgsl_pwrctrl sirasi (rail -> clk), mmcc GDSC bagimliliklari (oxili_gx parent = oxili_cx).
3. SP/TP power-on bekleme: A430 yamasindaki erken revizyon sorunuyla iliskili (RBBM_CLOCK_CTL); cihaz v2.1 (0x04030002) oldugundan bu yol gerekmeyebilir. Test: hack'siz hw_init + clock gating ayari.
4. a4xx register dump'ta TZ SEA: dump listesinden XPU/VMIDMT araliklarini cikarmak kalici dogru cozum (sadece A430 icin).
5. OCMEM: GMEM tahsisi GX sonrasi; ocmem 8994 quirk'i temizlenmeli.
6. Mesa tarafi: freedreno a4xx ile gercek render dogrulanmadi -> ilk test kmscube/glmark2-es2.

## Onerilen calisma yontemi
A) Hizli yol: fork master+camss'i taban al, derle, aygitta dene (rescue icin USB gadget gerekli: UC120 + usb3 high-speed).
B) Temiz yol: yamalari mainline'a seri halinde rebase et (etiketli olanlari once temizle). A ile baslayip B'ye gecmek mantikli.
Sonraki is: kernel'i derleyip (aarch64 cross) fork master+camss birlesimini cikarmak ve acik cakismalari raporlamak.
