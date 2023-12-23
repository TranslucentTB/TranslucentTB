$ErrorActionPreference = "Stop"

$is_ci = $env:TF_BUILD -eq "True"
$is_shallow = git rev-parse --is-shallow-repository
$tag_hash = git rev-list --tags --max-count=1
$head_hash = git rev-parse HEAD

if ($is_shallow -eq "true")
{
	if ($is_ci)
	{
		if ($env:BUILD_REASON -eq "PullRequest")
		{
			$repo_name = (Invoke-RestMethod -Uri "https://api.github.com/repos/$env:BUILD_REPOSITORY_NAME/pulls/$env:SYSTEM_PULLREQUEST_PULLREQUESTNUMBER").head.repo.full_name
		}
		else
		{
			$repo_name = $env:BUILD_REPOSITORY_NAME
		}
	}
	else
	{
		$upstream = git rev-parse --abbrev-ref --symbolic-full-name "@{u}"
		if (-not $?)
		{
			throw "You are on a detached HEAD"
		}

		$remote_name = $upstream.Substring(0, $upstream.IndexOf("/"))
		$remote_url = git config --get remote.$remote_name`.url

		if ($remote_url -match "github\.com\/([\w-\.]+\/[\w-\.]+)")
		{
			$repo_name = $matches[1]
			if ($repo_name.EndsWith(".git"))
			{
				$repo_name = $repo_name.Substring(0, $repo_name.LastIndexOf("."))
			}
		}
	}

	$commits_since_tag = (Invoke-RestMethod -Uri "https://api.github.com/repos/$repo_name/compare/$tag_hash...$head_hash").ahead_by
}
else
{
	$commits_since_tag = git rev-list $tag_hash`..HEAD --count
}

$tag = git describe --tags $tag_hash
$head_short = $head_hash.Substring(0, 7)

$short_version = "$tag.$commits_since_tag.0"
$full_version = "$tag.$commits_since_tag.$head_short"

$info_file = "Common\appinfo.hpp"
$info_content = (Get-Content $info_file)
$info_content = $info_content.Replace("1,0,0,1", $short_version.Replace(".", ","))
$info_content = $info_content.Replace("1.0.0.1", $full_version)
Set-Content $info_file -Value $info_content

$appx_file = "AppPackage\Package.appxmanifest"
$appx_content = (Get-Content $appx_file).Replace("1.0.0.0", $short_version)
Set-Content $appx_file -Value $appx_content

if ($is_ci)
{
	Write-Output "##vso[build.updatebuildnumber]$full_version"
}
