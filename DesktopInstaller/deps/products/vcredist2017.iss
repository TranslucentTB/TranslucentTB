; requires Windows 10, Windows 7 Service Pack 1, Windows 8, Windows 8.1, Windows Server 2003 Service Pack 2, Windows Server 2008 R2 SP1, Windows Server 2008 Service Pack 2, Windows Server 2012, Windows Vista Service Pack 2, Windows XP Service Pack 3
; http://www.visualstudio.com/en-us/downloads/

[CustomMessages]
vcredist2017_title=Visual C++ 2017 Redistributable
vcredist2017_size=13.7 MB

[Code]
const
	vcredist2017_url = 'http://download.microsoft.com/download/1/f/e/1febbdb2-aded-4e14-9063-39fb17e88444/vc_redist.x86.exe';
	vcredist2017_upgradecode = '{65E5BD06-6392-3027-8C26-853107D3CF1A}';

procedure vcredist2017(minVersion: string);
begin
	if (not msiproductupgrade(vcredist2017_upgradecode, minVersion)) then
		AddProduct('vcredist2017.exe',
			'/passive /norestart',
			CustomMessage('vcredist2017_title'),
			CustomMessage('vcredist2017_size'),
			vcredist2017_url,
			false, false, false);
end;

[Setup]
