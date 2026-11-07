# Lumia 950 XL (cityman, RM-1085): DSDT cihaz envanteri

Kaynak: stock W10M FFU 10586.13169, PLAT\ACPI\dsdt.aml (iasl 20200925 ile çözüldü). 128 aygıt.

| Aygıt | HID | UID | MMIO base/len | IRQ (GIC SPI+32) | GPIO (tür:kaynak:pin) | I2C (adr@hız → denetleyici) |
|---|---|---|---|---|---|---|
| \_SB.SDC1 | QCOM24BF | Zero | 0xF9824900/0x00000200 | 0x0000009B |  |  |
| \_SB.SDC1.EMMC | _ADR 0x08 |  |  |  |  |  |
| \_SB.SDC2 | QCOM2466 | One | 0xF98A4900/0x00000200 | 0x0000009D |  |  |
| \_SB.ABD | QCOM2431 | Zero |  |  |  |  |
| \_SB.SPMI | QCOM2404 | One | 0xFC4C0000/0x00100000 |  |  |  |
| \_SB.GIO0 | QCOM2405 | Zero | 0xFD510000/0x00002000 | 0x000000F0 0x000000F0 0x000000F0 |  |  |
| \_SB.PMIC | QCOM2455 |  |  |  |  |  |
| \_SB.PM01 | QCOM2458 | One |  | 0x000000DE |  |  |
| \_SB.PM02 | QCOM2458 | 0x02 |  | 0x000000DE |  |  |
| \_SB.PMBT | QCOM2453 |  |  |  |  |  |
| \_SB.PMBM | QCOM2452 |  |  |  |  |  |
| \_SB.PBTD | QCOM24C3 |  |  |  |  |  |
| \_SB.PMAP | QCOM2457 |  |  |  | Int:PM02:0x08 |  |
| \_SB.PRTC |  |  |  |  |  | 0x0002@0→ABD |
| \_SB.PMPB | QCOM2459 |  |  |  |  |  |
| \_SB.PMBD | QCOM24C2 |  |  |  |  |  |
| \_SB.BIFA | MSHW1002 |  |  |  |  |  |
| \_SB.PEP0 | QCOM2425 |  |  | 0x000000D8 0x000000C8 0x000000C9 0x000000CB |  | 0x0001@0→ABD |
| \_SB.BAM0 | QCOM2401 | Zero | 0xF9984000/0x00015000 | 0x0000007E |  |  |
| \_SB.BAM1 | QCOM2401 | One | 0xFD404000/0x00009000 | 0x000000EF |  |  |
| \_SB.BAM2 | QCOM2401 | 0x02 | 0xFD444000/0x00009000 | 0x0000010C |  |  |
| \_SB.BAM3 | QCOM2401 | 0x03 | 0xF9904000/0x00019000 | 0x0000010E |  |  |
| \_SB.BAM4 | QCOM2401 | 0x04 | 0xF9944000/0x00019000 | 0x0000010F |  |  |
| \_SB.BAM5 | QCOM2401 | 0x05 | 0xFE184000/0x00014000 | 0x00000144 |  |  |
| \_SB.BAM6 | QCOM2401 | 0x06 | 0xFE104000/0x00020000 | 0x000000C4 |  |  |
| \_SB.BAM7 | QCOM2401 | 0x07 | 0xF99C4000/0x00011000 | 0x0000009A |  |  |
| \_SB.BAM8 | QCOM2401 | 0x08 | 0xFD3C4000/0x00009000 | 0x00000149 |  |  |
| \_SB.BAMC | QCOM2401 | 0x0C | 0xF9A44000/0x00011000 | 0x000000A7 |  |  |
| \_SB.BAMD | QCOM2401 | 0x0D | 0xF9304000/0x00009000 | 0x000000A4 |  |  |
| \_SB.BAME | QCOM2401 | 0x0E | 0xFD4C4000/0x00015000 | 0x0000014C |  |  |
| \_SB.UAR2 | QCOM2424 | 0x02 | 0xF991E000/0x00000100 | 0x0000008C |  |  |
| \_SB.UAR8 | QCOM2424 | 0x08 | 0xF995E000/0x00000100 | 0x00000092 |  |  |
| \_SB.I2C1 | QCOM240E | One | 0xF9923000/0x00000800 | 0x0000007F |  |  |
| \_SB.I2C2 | QCOM240E | 0x02 | 0xF9924000/0x00000800 | 0x00000080 |  |  |
| \_SB.I2C6 | QCOM240E | 0x06 | 0xF9928000/0x00000800 | 0x00000084 |  |  |
| \_SB.I2C7 | QCOM240E | 0x07 | 0xF9963000/0x00000800 | 0x00000085 |  |  |
| \_SB.IC11 | QCOM240E | 0x0B | 0xF9967000/0x00000800 | 0x00000089 |  |  |
| \_SB.SP10 | QCOM240C | 0x0A | 0xF9966000/0x00000800 | 0x00000088 |  |  |
| \_SB.CDI | QCOM245B |  |  |  |  |  |
| \_SB.RPEN | QCOM245C |  |  |  |  |  |
| \_SB.PILC | QCOM2429 |  |  |  |  |  |
| \_SB.ADSP | QCOM242B |  |  | 0x000000C2 |  |  |
| \_SB.ADSP.SLM1 |  |  | 0xFE12F000/0x0002C000 | 0x000000C3 |  |  |
| \_SB.ADSP.SLM1.ADCM |  |  |  |  |  |  |
| \_SB.ADSP.SLM1.ADCM.AUDD |  |  |  |  |  | 0x0040@400000→I2C2 |
| \_SB.ADSP.SLM1.ADCM.AUDD.VISA | MSHW1019 | One |  |  |  | 0x0040@400000→I2C2 |
| \_SB.ADSP.SLM1.ADCM.AUDD.MBHC |  |  |  |  |  |  |
| \_SB.ADSP.SLM1.ADCM.AUDD.QCRT |  |  |  |  |  |  |
| \_SB.ADSP.SLM1.ATTS | MSHW1018 |  |  |  |  |  |
| \_SB.ADSP.SLM2 |  |  | 0xFE1AF000/0x0002C000 | 0x00000143 |  |  |
| \_SB.AMSS | QCOM242C |  |  | 0x00000038 |  |  |
| \_SB.QMUX | QCOM244F |  |  |  |  |  |
| \_SB.QSM | QCOM24B8 |  | 0x0E900000/0x00600000 |  |  |  |
| \_SB.MMU0 | QCOM2400 | Zero | 0xFD9CC000/0x0000B000 | 0x00000069 0x0000004F 0x0000004E 0x00000050 |  |  |
| \_SB.MMU1 | QCOM2400 | One | 0xFDA44000/0x0000B000 | 0x0000005E 0x00000060 0x00000061 0x00000062 |  |  |
| \_SB.MMU2 | QCOM2400 | 0x02 | 0xFDA64000/0x0000C000 | 0x00000063 0x00000065 0x00000066 0x00000067 |  |  |
| \_SB.MMU3 | QCOM2400 | 0x03 | 0xFDB10000/0x0000B000 | 0x00000046 0x00000111 0x00000112 0x00000110 |  |  |
| \_SB.MMU4 | QCOM2400 | 0x04 | 0xFDC84000/0x0000D000 | 0x0000004D 0x0000004A 0x0000004B 0x00000049 |  |  |
| \_SB.MMU5 | QCOM2400 | 0x05 | 0xFDA84000/0x0000B000 | 0x00000128 0x00000127 0x0000012A 0x0000012B |  |  |
| \_SB.MMU6 | QCOM2400 | 0x06 | 0xFD864000/0x0000B000 | 0x0000015A 0x0000015D 0x0000015E 0x0000015F |  |  |
| \_SB.OMEM | QCOM2403 | Zero | 0xFDD00000/0x00004000; 0xFE039000/0x00000400 | 0x0000006D |  |  |
| \_SB.DISP | MSHW1004 | Zero |  |  |  |  |
| \_SB.GPU0 | QCOM246F | Zero |  |  |  |  |
| \_SB.GPU0.MON0 | _ADR Zero |  |  |  |  |  |
| \_SB.GPU0.AVS0 |  |  |  |  |  |  |
| \_SB.SCM0 | QCOM2402 | Zero |  |  |  |  |
| \_SB.TREE | QCOM24BA | Zero |  |  |  |  |
| \_SB.PCI0 |  | Zero | 0xF8A00000/0x00500000 |  |  |  |
| \_SB.PCI0.RP1 |  |  |  |  |  |  |
| \_SB.PCI0.RP1.ENDP |  |  |  |  |  |  |
| \_SB.SMD0 | QCOM2407 | Zero | 0x06A00000/0x00200000; 0xFD484000/0x00001000; 0xFE805FF0/0x00000010 | 0x00000039 0x000000BC 0x0000003B 0x000000BE |  |  |
| \_SB.IPC0 | QCOM240A |  |  |  |  |  |
| \_SB.RFS0 | QCOM2423 |  | 0x88888888/0x99999999; 0x11111111/0x22222222; 0x33333333/0x44444444 |  |  |  |
| \_SB.IPA | QCOM24B0 | Zero |  | 0x0000014D |  |  |
| \_SB.QDIG | QCOM2413 |  |  |  |  |  |
| \_SB.QCDB | QCOM248B |  |  |  |  |  |
| \_SB.SSM | QCOM2414 |  |  |  |  |  |
| \_SB.CPU0 |  | Zero |  |  |  |  |
| \_SB.CPU1 |  | One |  |  |  |  |
| \_SB.CPU2 |  | 0x02 |  |  |  |  |
| \_SB.CPU3 |  | 0x03 |  |  |  |  |
| \_SB.CPU4 |  | 0x04 |  |  |  |  |
| \_SB.CPU5 |  | 0x05 |  |  |  |  |
| \_SB.CPU6 |  | 0x06 |  |  |  |  |
| \_SB.CPU7 |  | 0x07 |  |  |  |  |
| \_SB.GPS | QCOM24B4 | Zero |  |  |  |  |
| \_SB.ARPC | QCOM248A |  |  |  |  |  |
| \_SB.QDSS | QCOM247D | Zero | 0xFC300000/0x00080000; 0xFA000000/0x01000000; 0xFB800000/0x00800000 | 0x0000012E 0x00000168 0x00000169 0x0000016A |  |  |
| \_SB.QBCC | QCOM241D | Zero | 0xF9BFF000/0x00000124; 0xFD45A000/0x0000500B; 0xFD41A000/0x0000500B |  |  |  |
| \_SB.COEX | QCOM2487 |  |  |  |  |  |
| \_SB.TECC | MSHW1024 | Zero |  |  |  |  |
| \_SB.TSC1 | MSHW1003 | One |  |  |  | 0x004B@400000→I2C1 |
| \_SB.AGR0 |  |  |  |  |  |  |
| \_SB.CAMP | QCOM245E | 0x1B | 0xFDA0C000/0x00003FFF; 0xFDA00000/0x00000047 | 0x00000052 |  |  |
| \_SB.CAMS | QCOM2434 | 0x15 | 0xFDA08000/0x000003FF; 0xFDA0AC00/0x000003FF | 0x00000053 |  |  |
| \_SB.CAMF | QCOM2439 | 0x1A | 0xFDA08800/0x000003FF; 0xFDA0B400/0x000003FF | 0x00000055 |  |  |
| \_SB.CAMT | QCOM2436 | 0x1C | 0xFDA08C00/0x000003FF | 0x00000056 |  |  |
| \_SB.FLSH | QCOM244B | 0x19 |  |  |  |  |
| \_SB.FLHT | QCOM244C | 0x1D |  |  |  |  |
| \_SB.JPGE | QCOM2467 | 0x17 | 0xFDA1C000/0x00000320; 0xFDA60000/0x00000C34; 0xFDA20000/0x00000320 | 0x0000005B 0x0000005C 0x00000150 |  |  |
| \_SB.VFE0 | QCOM2432 | 0x16 | 0xFDA0A000/0x00000BFF; 0xFDA40000/0x00001FFF; 0xFDA18000/0x00003FFF | 0x00000059 0x00000057 0x00000051 0x0000005A |  |  |
| \_SB.BTNS |  | Zero |  |  | Int:PM01:0x40; Int:PM01:0x40 |  |
| \_SB.HWN1 | QCOM24A4 | One |  |  |  |  |
| \_SB.HWN0 | QCOM24A3 | Zero |  |  |  |  |
| \_SB.RVRM | QCOM2498 |  |  |  |  |  |
| \_SB.LNK0 | QCOM24A7 | Zero |  |  |  |  |
| \_SB.LNK1 | QCOM24A7 | One |  |  |  |  |
| \_SB.LNK2 | QCOM24A7 | 0x02 |  |  |  |  |
| \_SB.LNK3 | QCOM24A7 | 0x03 |  |  |  |  |
| \_SB.LNK4 | QCOM24A7 | 0x04 |  |  |  |  |
| \_SB.LNK5 | QCOM24A7 | 0x05 |  |  |  |  |
| \_SB.LNK6 | QCOM24A7 | 0x06 |  |  |  |  |
| \_SB.LNK7 | QCOM24A7 | 0x07 |  |  |  |  |
| \_SB.LNK8 | QCOM24A7 | 0x08 |  |  |  |  |
| \_SB.MBRG | QCOM2145 |  |  |  |  |  |
| \_SB.RMAT | QCOM2155 |  |  |  |  |  |
| \_SB.RMNT | QCOM2148 |  |  |  |  |  |
| \_SB.SEN1 | QCOM2495 |  |  |  |  |  |
| \_SB.QDCI | QCOM2412 |  |  |  |  |  |
| \_SB.FMSL | QCOM24B1 |  |  |  |  | 0x0011@400000→IC11 |
| \_SB.BTH0 | QCOM24B2 |  |  |  |  |  |
| \_SB.URS0 | QCOM24B6 |  | 0xF9200000/0x000FFFFF |  |  |  |
| \_SB.URS0.USB0 |  |  |  | 0x000000A3 0x000000D4 |  |  |
| \_SB.URS0.UFN0 |  |  |  | 0x000000A3 0x000000D4 |  |  |
| \_SB.UFN1 | QCOM01C0 | One | 0xF9A55000/0x00000300 | 0x000000A6 0x000000AC |  |  |
| \_SB.PER0 | QCOM25A0 |  |  |  |  |  |
| \_SB.ADC1 | QCOM240F | Zero |  |  |  |  |
| \_SB.ADC2 | QCOM240F | One |  |  |  |  |