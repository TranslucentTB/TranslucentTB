$ErrorActionPreference = "Stop"

if ($env:PLATFORM -eq "x86") {
    $bin = $env:CONFIGURATION
} else {
    $bin = Join-Path $env:PLATFORM  $env:CONFIGURATION
}

Copy-Item -Path $bin -Destination "Out" -Recurse