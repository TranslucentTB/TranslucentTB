param(
    [string] $Platform,
	[string] $Configuration,
	[string] $ArtifactSigningMetadata,
	[string] $DigestAlgorithm,
	[string] $TimestampServer
)

$ErrorActionPreference = "Stop"

$msixPackageFolder = (Get-ChildItem "$PSScriptRoot\..\..\AppPackage\AppPackages").FullName
$msixPackage = (Get-ChildItem "$msixPackageFolder" -Filter "*.msix").FullName

[xml]$config = Get-Content "$PSScriptRoot\..\..\TranslucentTB\packages.config"

$artifactSigningClientVersion = ($config.packages.package | Where { $_.id -eq "Microsoft.ArtifactSigning.Client" }).version
$sdkVersion = ($config.packages.package | Where { $_.id -eq "Microsoft.Windows.SDK.BuildTools" }).version

$artifactSigningClientFolder = "$PSScriptRoot\..\..\packages\Microsoft.ArtifactSigning.Client.$artifactSigningClientVersion\bin"
$sdkFolder = (Get-ChildItem "$PSScriptRoot\..\..\packages\Microsoft.Windows.SDK.BuildTools.$sdkVersion\bin").FullName

&"$sdkFolder\x64\signtool.exe" sign /ph /tr "$TimestampServer" /td $DigestAlgorithm /fd $DigestAlgorithm /dlib "$artifactSigningClientFolder\x64\Azure.CodeSigning.Dlib.dll" /dmdf "$ArtifactSigningMetadata" "$msixPackage"
