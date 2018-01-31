$content = (Get-Content TranslucentTB\TranslucentTB.rc)

$tag = git describe --abbrev=0 --tags
$tag_hash = git show-ref -s $tag
$commits = git rev-list $tag_hash --count
$tags = (git tag | Measure-Object -Line).Lines

$content = $content.Replace("1,0,0,1", "$tags,$commits,0,0")
$content = $content.Replace("1.0.0.1", "$tag.$commits.$env:APPVEYOR_REPO_COMMIT.$env:APPVEYOR_REPO_BRANCH-$env:CONFIGURATION")

Set-Content TranslucentTB\TranslucentTB.rc -Value $content