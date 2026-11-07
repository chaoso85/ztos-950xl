# GPU (Adreno 430) ve Kamera: kaynak birlestirme raporu (2026-11-07)

Kaynaklar: mainline torvalds/linux (master), Android4Lumia950/linux (master, camss, mdss, iommu, msm8994-6.x dallari),
Android4Lumia950/android_kernel_mmo_msm8994 (downstream, msm8994-v2.1.dtsi, gpu/camera dtsi), bu FFU (DSDT, qccam*.sys, .dcc, SOFTWARE hive).
"DOGRULANDI" = kaynak kodda/dosyada gordum. "CIKARIM" = kanitlardan sonuc, donanimda test edilmedi.

## 1. GPU: Linux Adreno 4xx sürücüsü ne kadar tamam?

### Mainline (torvalds/linux master)
- DOGRULANDI: drm/msm a4xx_gpu.c, A405/A420/A430 icin kod var. A430 tek chip id ile kayitli: 0x04030002. Firmware: a420_pm4.fw / a420_pfp.fw (linux-firmware icinde, WHENCE'te listeli).
- DOGRULANDI: Lumia'nin kullandigi downstream msm8994-v2.1.dtsi chip id'yi 0x04030002 yapiyor (temel msm8994-gpu.dtsi'de 0x04030000). Yani 950 XL (v2.1) mainline kaydiyla eslesiyor. GPU 630 MHz'e kadar.
- DOGRULANDI: mmcc-msm8994 icinde oxili_gfx3d_clk, oxili_rbbmtimer_clk, oxilicx_ahb_clk ve OXILI_CX/GX GDSC var. MDP5 icin msm8x94 konfigi (rev 9) ve DSI 6G host + msm8994 regulator tablosu var. DSI 20nm PHY kodu var ama CONFIG_DRM_MSM_DSI_20NM_PHY gerekiyor.
- YOK: mainline msm8994.dtsi'de GPU/MDSS dugumu yok.
- freedreno posta listesinde (Aralik 2015) A430 destegi "UNTESTED" yama olarak onerilmisti; sonraki test/bakim gecmisini dogrulamadim, A430'un mainline'da sagligi icin kanitim yok.

### Android4Lumia950/linux (master, son commit 2026-09-10)  -> asil is burada
Cityman icin GPU+MDSS DT'de etkinlestirilmis ("Enable GPU and MDSS", "Add GPU node", "Add MDSS nodes and its DSIs", "Add iommus", "Add interconnects"). Surucu tarafinda iki commit var ve ikisi de acikca gecici/kirli isaretli:
1. e8d7ff26a "A4xx: Support A430 for msm8994": chip id 0x04030000 eklenmis; her submit oncesi RBBM_POWER_CNTL_IP yazip SP/TP power-on bekleniyor; a4xx register dump'i A430'da TZ'nin SEA (hata) vermesi nedeniyle kisaltilmis; OCMEM alloc'u GX acildiktan sonraya tasinmis.
2. 6b36f290f "[DIRTY] Add A430 hacks for proper init": soc/qcom/msm8994-oxili.h ile ozel on-guc helper'i; ilk GEM adresi 16 MiB yerine 32 MiB (16 MiB'de memptrs izin hatasi veriyor); runtime PM'de put_autosuspend yapilmiyor (GPU hep acik kaliyor, aksi halde SoC SEA veriyor). Ek: "rpmpd: Modify s2b corner for GPU".
- CIKARIM: GPU ilk hw_init'i geciyor ve ring fetch retire oluyor (commit mesajlarina gore) ama (a) guc yonetimi calismiyor (GPU hic uyumuyor, pil), (b) IOMMU/TZ ile ilgili temel sorun (SCM(18) GX kapaliyken -EINVAL) duzgun cozulmemis, sadece atlatilmis.
- DOGRULANMADI: Mesa ile gercek render (kmscube/glmark) sonucu; GPU hang orani; bu commit'lerin ardindan calisan bir userspace testi.
- msm8994-6.x dalinin son notu (Aralik 2024): "GPU shows in dmesg, MDSS hangs the kernel". Master'daki ilerleme bundan sonraki (Ağustos-Eylül 2026).

### Sonuc ve "kusursuz port" yolu
GPU "yari calisiyor" degerlendirmen dogru: calisan ama guc yonetimsiz, hack'li. Plan:
1. Calisan temeli (Android4Lumia950 master) al, upstream'e uygun hale getir: hack'leri tek tek kok nedene indir.
2. Kok neden kaynagi: downstream kgsl 3.10 (power on sirasi: rail -> clock -> SMMU/TBU, OCMEM/GMEM, secure iommu scm) ve Windows qcdxkm8994.sys. FFU'da elimizde.
3. Test seti: kmscube, glmark2-es2, ztMETRO; hang/idle/suspend dongusu 1000x.
4. Firmware: a420_pm4/pfp linux-firmware'den; kurulumda indirilir/aygitta paketlenir (yeniden dagitim lisansi WHENCE'te kontrol edilecek).

## 2. Kamera

### Donanim (FFU'dan, DOGRULANDI)
- 3 kamera: arka (RFC, OIS'li, ~20MP, DNG var), on (FFC), "tertiary" = iris tarayici IR kamerasi (tertiary flash = IR aydinlatma). Windows suruculeri: qccamrearsensor_primarySMIApp, qccamfrontsensor_secondarySMIApp, qccamsensor_tertiarySMIApp, qccamflash / qccamflashtertiary.
- Sensorler "SMIA++" (MIPI SMIA/CCS ailesi) sarmalayicisi (SMIApp) ile suruluyor; suruculer sensoru NVM'den okunan kamera kimligi dizgisiyle eslesen .dcc (CDCC) dosyasindan yapilandiriyor. 21 .dcc dosyasi: 0A2140xx (Ducati ES1.0, ~1.4 MB), 0AEACAxx (Cooper MP2.0 / Carrera CS5.5), 0C086101 (Jopo/Foxconn), 104541xx (Karma/Mitsumi, BU24210 ailesi, baska model ailesine ait gorunuyor). Ayrinti: camera/DCC_INVENTORY.md.
- Kayit defteri (camera/REGISTRY_OEM_MMO_Camera.txt): arka I2C 1000 kHz, FlipMirror, max ISO 3200, OIS var; on kamera FlipMirror=1; tertiary cropWidthLeft=300.
- SoC blok adresleri (DSDT == downstream DT, DOGRULANDI): CCI 0xFDA0C000 (IRQ SPI50), CSID0..3 0xFDA08000/08400/08800/08C00 (SPI51..54), CSIPHY0..2 0xFDA0AC00/0B000/0B400 (SPI78..80), ISPIF 0xFDA0A000 (SPI55), VFE0 0xFDA10000 / VFE1 0xFDA14000 (SPI57/58, "vfe46"), CPP 0xFDA04000, JPEG 0xFDA1C000/20000/24000, IRQ router 0xFDA00000.
- DSDT CAMP GPIO listesi: 92, 91, 102, 106, 104, 105, 12, 8, 21 (+ bir PMIC/ek denetleyici pini 0x508). Reset/standby ciftleri: (92,91) arka, (104,105) on, (102,106) iris. Bu esleme downstream MTP referansiyla ve camss dalindaki DT ile tutarli.

### Android4Lumia950/linux `camss` dali (DOGRULANDI)
- CAMSS_8x94 eklenmis: csiphy/csid/ispif/vfe kaynak tablolari (csid_ops_4_1, csiphy_ops_2ph_1_0), cityman DT'de cci, camss, uc sensor dugumu ("nokia,smia"): Ducati on (@0x20, cci_i2c0, reset 104, vana l17a 2.7V), Siiri iris (@0x6c, Toshiba T4KE1, reset 102), Hill arka (@0x20, cci_i2c1, Sony IMX230, reset 92, vana l29a). Son commit mesaji: "CAMSS and CCI are up(?) ccs not yet."
- Bu isimlendirme FFU ile uyumlu: surucuda "Found a Siiri sensor", qcadcm8994.sys icinde T4KE, DCC'lerde Ducati/Carrera. IMX230 FFU'da dogrudan gecmedi (CIKARIM, calisan DT notuna dayaniyor).

### Eksik olan = CCS/sensor katmani
- Mainline'da MIPI CCS surucusu (drivers/media/i2c/ccs) var; ancak cep telefonu sensorleri (ozellikle IMX230: PDAF, OIS) tam CCS uyumlu olmayabilir. "ccs not yet" bunun henuz calismadigini soyluyor.
- Windows tarafi: mod tablolari ve vendor register yazimlari .dcc icinde ("Register 0x0524 added at the end of OIS VF/VIDEO/STILL MODES"). Bu, saf CCS ile temel akisin (tek mod preview) calisabilecegini ama tam mod seti icin dcc'den cikarilan bir tablo katmani gerekebilecegini gosteriyor (CIKARIM).
- OIS (Rohm BU24210-ailesi ve Windows OIS FW yukleme), AF aktuatoru, flas (AS3638 uclu LED surucusu, "no driver available" notu) ayri suruculer gerektiriyor.
- DOGRULANMADI/celiski: camss DT'sinde MCLK atamasi (on=MCLK1/gpio14, iris=MCLK2/gpio15) Qualcomm MTP referansiyla (gpio15=MCLK2 ile 104/105) celisiyor. Gercek atama canli test veya Windows PEP tablosu ile dogrulanmali.
- DOGRULANMADI: link-frequencies 342 MHz; .dcc'de bir modda 336 MHz CSI saati geciyor (yakin ama ayni degil).

### Kamera icin yapilacaklar (sirali)
1. Android4Lumia950 camss + cityman DT'yi bizim taban cekirdege al; CCI i2c tarama (i2cdetect 0x20/0x6c) ile sensor varligini dogrula.
2. Sensor ID registerlarini oku (0x0000-0x0006 CCS standart) -> model/rev; FFU'daki camera ID dizgisiyle karsilastir.
3. ccs surucusu ile tek mod akis; basarisizsa cikis: .dcc mod tablosu ayristirici (format dokumani yok; ters muhendislik).
4. Sonra AF/OIS/flas; en son libcamera pipeline (soft ISP) - donanim ISP tuning'i (Qualcomm) kullanilmayacak.
5. Iris kamerasi ztOS icin onemsiz; en sona.

## 3. Gozlemler (USB / diger, kısa)
- Downstream cityman DT: usb3 maximum-speed = high-speed (950 XL'de SS yok), hsphy init degerleri 0x00D191A4 / 0x00D1D9A4, pcie0 kapali, pcie1 kullaniliyor (Wi-Fi). Master'da UC120 CC PHY ve SMBCHG commit'leri de var => USB gadget icin hazir kod var.
- Fork master: Wi-Fi/ADSP(remoteproc)/ses (WCD9330, TAS2552) commit'leri var; durumlari test edilmedi.

## 4. Onerilen sonraki adim
Android4Lumia950/linux master (+camss) dalini tam cekip: (a) mainline'a gore diff'ini sayisal cikar, (b) bizim tabanimiz olarak sec, (c) GPU hack'lerini kok nedene indirme listesini cikar. Baska bir secenek: tabani mainline tutup yamalari tek tek rebase etmek (daha temiz, daha yavas).
