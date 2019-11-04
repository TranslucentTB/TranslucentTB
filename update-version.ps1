$ErrorActionPreference = "Stop"

$tag = git describe --abbrev=0 --tags
$tag_commas = $tag.Replace(".", ",")
$tag_hash = git show-ref -s $tag
$commits_since_tag = git rev-list $tag_hash`...HEAD --count
$current_commit = $env:APPVEYOR_REPO_COMMIT.Substring(0, 7)

$exe_short_version = "$tag_commas,$commits_since_tag,0"
$exe_full_version = "$tag.$commits_since_tag.$current_commit"
$appx_short_version = "$tag.$commits_since_tag.0"

$info_file = "Common\appinfo.hpp"
$info_content = (Get-Content $info_file)
$info_content = $info_content.Replace("1,0,0,1", $exe_short_version)
$info_content = $info_content.Replace("1.0.0.1", $exe_full_version)
Set-Content $info_file -Value $info_content

$appx_file = "AppPackage\Package.appxmanifest"
$appx_content = (Get-Content $appx_file).Replace("1.0.0.0", $appx_short_version)
Set-Content $appx_file -Value $appx_content

Update-AppveyorBuild -Version $exe_full_version