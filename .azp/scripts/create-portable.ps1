param(
    [string] $Platform,
	[string] $Configuration,
	[string] $WorkDir,
	[string] $OutputDir,
	[switch] $NoConfig,
	[switch] $Compress
)

$ErrorActionPreference = "Stop"

New-Item -Path "$WorkDir" -Name "portable-$Platform-$Configuration" -ItemType "Directory" -Force
New-Item -Path "$OutputDir" -ItemType "Directory" -Force

Copy-Item -Path "$PSScriptRoot\..\..\AppPackage\bin\$Platform\$Configuration\*" -Destination $(if ($Compress) {"$WorkDir\portable-$Platform-$Configuration"} else {"$OutputDir"}) -Recurse -Include @("*.exe", "*.dll", "resources.pri", "Assets")

if ($Compress)
{
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

	Compress-Archive -Path "$WorkDir\portable-$Platform-$Configuration\*" -DestinationPath "$OutputDir\$filename"
}

Remove-Item -Path "$WorkDir\portable-$Platform-$Configuration" -Recurse
