$ErrorActionPreference = "Stop"

$appx_file = "AppPackage\Package.appxmanifest"
$appx_content = Get-Content $appx_file
$original_subject = "CN=TranslucentTB Open Source Developers, OID.2.25.311729368913984317654407730594956997722=1"

if ($env:BUILD_TYPE -eq "Release")
{
	$appx_content = $appx_content.Replace("TranslucentTB (Dev)", "TranslucentTB");
	$appx_content = $appx_content.Replace("ms-resource:PublisherDisplayName", "Charles Milette");
	$appx_content = $appx_content.Replace($original_subject, "CN=04797BBC-C7BB-462F-9B66-331C81E27C0E");
}
else if ($env:BUILD_TYPE -eq "Canary")
{
	$appx_content = $appx_content.Replace("TranslucentTB (Dev)", "TranslucentTB (Canary)");
}

if ($env:BUILD_TYPE -ne "Release" -and $env:SYSTEM_PULLREQUEST_ISFORK -ne "True")
{
	$appx_content = $appx_content.Replace($original_subject, "CN=Charles Milette, O=Charles Milette, L=Greenfield Park, S=Quebec, C=CA");
}

Set-Content $appx_file -Value $appx_content
