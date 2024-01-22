$ErrorActionPreference = "Stop"

if ($env:BUILD_SOURCEBRANCH.StartsWith("refs/tags/"))
{
	$build_type = "Release"
}
else if ($env:BUILD_REASON -ne "PullRequest")
{
	$build_type = "Canary"
}
else
{
	$build_type = "Dev"
}

Write-Output "##vso[task.setvariable variable=build_type]$build_type"
