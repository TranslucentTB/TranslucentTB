param(
    [string] $Platform,
	[string] $Configuration,
	[string] $WorkDir,
	[string] $OutputDir,
	[switch] $NoConfig
)

$ErrorActionPreference = "Stop"

New-Item -Path $WorkDir -Name "portable-$Platform-$Configuration" -ItemType "Directory" -Force

Copy-Item -Path "$PSScriptRoot\..\AppPackage\bin\$Platform\$Configuration\*" -Destination "$WorkDir\portable-$Platform-$Configuration" -Recurse -Include @("*.exe", "*.dll", "resources.pri", "Assets")

$platform_lower = $Platform.ToLower()
if ($NoConfig)
{
	$filename = "TranslucentTB-portable-$platform_lower.zip"
}
else
{
	$configuration_lower = $Configuration.ToLower()
	$filename = "TranslucentTB-portable-$platform_lower-$configuration_lower.zip"
}

New-Item -Path $OutputDir -ItemType "Directory" -Force
Compress-Archive -Path "$WorkDir\portable-$Platform-$Configuration\*" -DestinationPath "$OutputDir\$filename"

Remove-Item -Path "$WorkDir\portable-$Platform-$Configuration" -Recurse
