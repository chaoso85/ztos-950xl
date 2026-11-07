<# Copies the ztOS boot files to an SD card (FAT32 partition, any drive letter).  Usage: .\prepare_sd.ps1 -Sd F: #>
param([Parameter(Mandatory)][string]$Sd, [string]$Src = "$PSScriptRoot\..\rescue_sd\ztos")
$ErrorActionPreference='Stop'
$vol = Get-Volume -DriveLetter $Sd.TrimEnd(':','\')
if ($vol.FileSystemType -notin 'FAT32','FAT') { throw "SD must be FAT32 (is $($vol.FileSystemType))" }
if ($vol.DriveType -ne 'Removable') { throw "Not a removable drive: refusing." }
$dst = $Sd.TrimEnd('\') + '\ztos'; New-Item -ItemType Directory -Force $dst | Out-Null
Copy-Item "$Src\*" $dst -Force
Get-ChildItem $dst | Format-Table Name, Length
