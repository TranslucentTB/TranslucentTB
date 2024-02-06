param(
    [string] $RepoName
)

$ErrorActionPreference = "Stop"

$tag_hash = git rev-list --tags --max-count=1
$tag_name = git describe --tags $tag_hash

$head_hash = git rev-parse HEAD
$head_short = $head_hash.Substring(0, 7)

$commits_since_tag = (Invoke-RestMethod -Uri "https://api.github.com/repos/$RepoName/compare/$tag_hash...$head_hash").ahead_by

$short_version = "$tag_name.$commits_since_tag.0"
$full_version = "$tag_name.$commits_since_tag.$head_short"

$info_file = "$PSScriptRoot\..\Common\appinfo.hpp"
$info_content = (Get-Content $info_file)
$info_content = $info_content.Replace("1,0,0,1", $short_version.Replace(".", ","))
$info_content = $info_content.Replace("1.0.0.1", $full_version)
Set-Content $info_file -Value $info_content

$appx_file = "$PSScriptRoot\..\AppPackage\Package.appxmanifest"
$appx_content = (Get-Content $appx_file).Replace("1.0.0.0", $short_version)
Set-Content $appx_file -Value $appx_content

Write-Output "##vso[build.updatebuildnumber]$full_version"
