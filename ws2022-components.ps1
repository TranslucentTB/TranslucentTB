Set-Location "C:\Program Files (x86)\Microsoft Visual Studio\Installer\"
$InstallPath = "C:\Program Files\Microsoft Visual Studio\2022\Preview"
$componentsToAdd = @(
  "Microsoft.VisualStudio.Component.VC.Runtimes.ARM64.Spectre",
  "Microsoft.VisualStudio.Component.VC.14.29.16.11.ARM",
  "Microsoft.VisualStudio.Component.VC.14.29.16.11.ARM.Spectre",
  "Microsoft.VisualStudio.Component.VC.14.29.16.11.ARM64",
  "Microsoft.VisualStudio.Component.VC.14.29.16.11.ARM64.Spectre",
  "Microsoft.VisualStudio.Component.VC.14.29.16.11.x86.x64",
  "Microsoft.VisualStudio.Component.VC.14.29.16.11.x86.x64.Spectre"
)
[string]$workloadArgs = $componentsToAdd | ForEach-Object {" --add " +  $_}
$Arguments = ('/c', "vs_installer.exe", 'modify', '--installPath', "`"$InstallPath`"",$workloadArgs, '--quiet', '--norestart', '--nocache')
$process = Start-Process -FilePath cmd.exe -ArgumentList $Arguments -Wait -PassThru -WindowStyle Hidden
if ($process.ExitCode -eq 0)
{
    Write-Host "components have been successfully added"
}
else
{
    Write-Host "components were not installed"
    exit 1
}
