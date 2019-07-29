$ErrorActionPreference = "Stop"

$tag = git describe --abbrev=0 --tags
$tag_hash = git show-ref -s $tag
$commits = git rev-list $tag_hash`...HEAD --count
$tags = (git tag | Measure-Object -Line).Lines

$info_file = Common\appinfo.hpp
$info_content = (Get-Content $info_file)
$info_content = $info_content.Replace("1,0,0,1", "$tags,$commits,0,0")
$info_content = $info_content.Replace("1.0.0.1", "$tag.$commits.$env:APPVEYOR_REPO_COMMIT.$env:APPVEYOR_REPO_BRANCH-$env:CONFIGURATION")
Set-Content $info_file -Value $info_content

$appx_file = AppPackage\Package.appxmanifest
$appx_content = (Get-Content $appx_file).Replace("1.0.0.0", "$tags.$commits.0.0")
Set-Content $appx_file -Value $appx_content
