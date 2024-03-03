param(
	[string] $OutputDir,
	[switch] $FromUpload,
	[string] $UploadOutputDir,
	[string] $WorkDir
)

$ErrorActionPreference = "Stop"

New-Item -Path "$OutputDir" -ItemType "Directory" -Force

if ($FromUpload)
{
	$upload = (Get-ChildItem "$PSScriptRoot\..\..\AppPackage\AppPackages" -Filter "*.msixupload").FullName

	New-Item -Path "$UploadOutputDir" -ItemType "Directory" -Force
	Copy-Item -Path "$upload" -Destination "$UploadOutputDir"

	New-Item -Path "$WorkDir" -Name "upload" -ItemType "Directory" -Force
	Copy-Item -Path "$upload" -Destination "$WorkDir\upload\msixupload.zip"
	Expand-Archive -Path "$WorkDir\upload\msixupload.zip" -DestinationPath "$WorkDir\upload"

	$symbols = Get-ChildItem "$WorkDir\upload" -Filter "*.appxsym"
	foreach ($appxSym in $symbols)
	{
		$output = $appxSym.Name.Substring($appxSym.Name.LastIndexOf("_") + 1).ToLower()
		Copy-Item -Path $appxSym.FullName -Destination "$OutputDir\$output"
	}

	Remove-Item -Path "$WorkDir\upload" -Recurse
}
else
{
	$msixPackageFolder = (Get-ChildItem "$PSScriptRoot\..\..\AppPackage\AppPackages").FullName
	$msixPackage = (Get-ChildItem $msixPackageFolder -Filter "*.msix").FullName
	$symbols = (Get-ChildItem $msixPackageFolder -Filter "*.appxsym").FullName

	Copy-Item -Path "$msixPackage" -Destination "$OutputDir"
	Copy-Item -Path "$symbols" -Destination "$OutputDir"
}
