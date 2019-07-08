Set-Location C:\tools\vcpkg

# Git decides to use stderr for regular output apparently. PowerShell no likey.
git pull 2>&1 | ForEach-Object { "$_" }
if ($LastExitCode -ne 0) { exit $LastExitCode }

.\bootstrap-vcpkg.bat
if ($LastExitCode -ne 0) { exit $LastExitCode }

.\vcpkg.exe integrate install
if ($LastExitCode -ne 0) { exit $LastExitCode }

.\vcpkg.exe install --triplet $env:PLATFORM-windows detours gtest rapidjson fmt
if ($LastExitCode -ne 0) { exit $LastExitCode }

.\vcpkg.exe install wil:$env:PLATFORM-windows --head
if ($LastExitCode -ne 0) { exit $LastExitCode }

.\vcpkg.exe install spdlog:$env:PLATFORM-windows --head
if ($LastExitCode -ne 0) { exit $LastExitCode }

Set-Location $env:APPVEYOR_BUILD_FOLDER

.\update-version.ps1
if ($LastExitCode -ne 0) { exit $LastExitCode }