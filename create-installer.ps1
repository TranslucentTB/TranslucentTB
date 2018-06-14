$ErrorActionPreference = "Stop"

if ($env:CONFIGURATION -eq "Release")
{
	& ".\DesktopInstallerBuilder\bin\Release\net462\DesktopInstallerBuilder.exe"
	Exit $LASTEXITCODE
}