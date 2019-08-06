$ErrorActionPreference = "Stop"

$tag = git describe --abbrev=0 --tags
$tag_commas = $tag.Replace(".", ",")
$tag_hash = git show-ref -s $tag
$commits_since_tag = git rev-list $tag_hash`...HEAD --count
$current_commit = $env:APPVEYOR_REPO_COMMIT.Substring(0, 7)

$info_file = "Common\appinfo.hpp"
$info_content = (Get-Content $info_file)
$info_content = $info_content.Replace("1,0,0,1", "$tag_commas,$commits_since_tag,0")
$info_content = $info_content.Replace("1.0.0.1", "$tag.$commits_since_tag.$current_commit")
Set-Content $info_file -Value $info_content

$appx_file = "AppPackage\Package.appxmanifest"
$appx_content = (Get-Content $appx_file).Replace("1.0.0.0", "$tag.$commits_since_tag.0")
Set-Content $appx_file -Value $appx_content
