param(
    [string] $Platform,
	[string] $Configuration,
	[string] $AcsMetadata,
	[string] $DigestAlgorithm,
	[string] $TimestampServer
)

$ErrorActionPreference = "Stop"

$msixPackageFolder = (Get-ChildItem "$PSScriptRoot\..\..\AppPackage\AppPackages").FullName
$msixPackage = (Get-ChildItem "$msixPackageFolder" -Filter "*.msix").FullName

[xml]$config = Get-Content "$PSScriptRoot\..\..\TranslucentTB\packages.config"

$acsClientVersion = ($config.packages.package | Where { $_.id -eq "Azure.CodeSigning.Client" }).version
$sdkVersion = ($config.packages.package | Where { $_.id -eq "Microsoft.Windows.SDK.BuildTools" }).version

$acsClientFolder = "$PSScriptRoot\..\..\packages\Azure.CodeSigning.Client.$acsClientVersion\bin"
$sdkFolder = (Get-ChildItem "$PSScriptRoot\..\..\packages\Microsoft.Windows.SDK.BuildTools.$sdkVersion\bin").FullName

&"$sdkFolder\x86\signtool.exe" sign /ph /tr "$TimestampServer" /td $DigestAlgorithm /fd $DigestAlgorithm /dlib "$acsClientFolder\x86\Azure.CodeSigning.Dlib.dll" /dmdf "$AcsMetadata" "$msixPackage"
