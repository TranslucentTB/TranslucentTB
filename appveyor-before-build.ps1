Set-Location C:\tools\vcpkg
git pull
.\bootstrap-vcpkg.bat
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }

.\vcpkg.exe integrate install
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }

.\vcpkg.exe install detours:$env:CONFIGURATION-windows
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }

.\vcpkg.exe install gtest:$env:CONFIGURATION-windows
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }

Set-Location $env:APPVEYOR_BUILD_FOLDER
.\update-version.ps1
if ($LastExitCode -ne 0) { $host.SetShouldExit($LastExitCode) }