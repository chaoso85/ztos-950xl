<# Installs the ztOS GRUB into the phone's EFIESP (phone in Mass Storage Mode, ESP given a drive letter).
   Windows stays bootable: the original bootmgfw.efi is kept as bootmgfw_real.efi and GRUB chainloads it.
   Do this ONLY after the eMMC backup is VERIFIED.   Usage: .\install_esp_grub.ps1 -Esp E: -Grub ..\rescue_sd\grubaa64.efi
   Undo:  .\install_esp_grub.ps1 -Esp E: -Restore #>
param([Parameter(Mandatory)][string]$Esp, [string]$Grub = "$PSScriptRoot\..\rescue_sd\grubaa64.efi", [switch]$Restore)
$ErrorActionPreference='Stop'
$esp = $Esp.TrimEnd('\') + '\'; $boot = Join-Path $esp 'EFI\Microsoft\Boot'
if (-not (Test-Path (Join-Path $boot 'BCD'))) { throw "$esp does not look like the phone EFIESP (no EFI\Microsoft\Boot\BCD)" }
$real = Join-Path $boot 'bootmgfw_real.efi'; $cur = Join-Path $boot 'bootmgfw.efi'
if ($Restore) { if (-not (Test-Path $real)) { throw 'nothing to restore' }; Copy-Item $real $cur -Force; Write-Host 'Restored original bootmgfw.efi (bootmgfw_real.efi kept).'; return }
if (-not (Test-Path $Grub)) { throw "GRUB image not found: $Grub" }
if (-not (Test-Path $real)) { Copy-Item $cur $real; Write-Host 'Saved original as bootmgfw_real.efi' }
New-Item -ItemType Directory -Force (Join-Path $esp 'EFI\ztos') | Out-Null
Copy-Item $Grub (Join-Path $esp 'EFI\ztos\grubaa64.efi') -Force
Copy-Item $Grub $cur -Force
(Get-FileHash $cur).Hash | Write-Host
Write-Host 'Done. No SD card -> boots Windows as before. SD card with \ztos\grub.cfg -> ztOS menu.'
