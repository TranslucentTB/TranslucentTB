$ErrorActionPreference = "Stop"

$tag = git describe --abbrev=0 --tags
$tag_hash = git show-ref -s $tag
$commits = git rev-list $tag_hash`...HEAD --count
$tags = (git tag | Measure-Object -Line).Lines

$rc_content = (Get-Content TranslucentTB\TranslucentTB.rc2)
$rc_content = $rc_content.Replace("1,0,0,1", "$tags,$commits,0,0")
$rc_content = $rc_content.Replace("1.0.0.1", "$tag.$commits.$env:APPVEYOR_REPO_COMMIT.$env:APPVEYOR_REPO_BRANCH-$env:CONFIGURATION")
Set-Content TranslucentTB\TranslucentTB.rc2 -Value $rc_content

$appx_content = (Get-Content AppPackage\Package.appxmanifest).Replace("1.0.0.0", "$tags.$commits.0.0")
Set-Content AppPackage\Package.appxmanifest -Value $appx_content