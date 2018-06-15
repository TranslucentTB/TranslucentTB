{
	--- TYPES AND VARIABLES ---
}
type
	TProduct = record
		File: String;
		Title: String;
		Parameters: String;
		ForceSuccess : boolean;
		InstallClean : boolean;
		MustRebootAfter : boolean;
	end;

	InstallResult = (InstallSuccessful, InstallRebootRequired, InstallError);

var
	installMemo, downloadMessage: string;
	products: array of TProduct;
	delayedReboot: boolean;
	DependencyPage: TOutputProgressWizardPage;

procedure AddProduct(filename, parameters, title, size, url: string; forceSuccess, installClean, mustRebootAfter : boolean);
{
	Adds a product to the list of products to download.
	Parameters:
		filename: the file name under which to save the file
		parameters: the parameters with which to run the file
		title: the product title
		size: the file size
		url: the URL to download from
		forceSuccess: whether to continue in case of setup failure
		installClean: whether the product needs a reboot before installing
		mustRebootAfter: whether the product needs a reboot after installing
}
var
	path: string;
	i: Integer;
begin
	installMemo := installMemo + '%1' + title + #13;

	path := ExpandConstant('{src}{\}') + CustomMessage('DependenciesDir') + '\' + filename;
	if not FileExists(path) then begin
		path := ExpandConstant('{tmp}{\}') + filename;

		if not FileExists(path) then begin
			isxdl_AddFile(url, path);

			downloadMessage := downloadMessage + '%1' + title + ' (' + size + ')' + #13;
		end;
	end;

	i := GetArrayLength(products);
	SetArrayLength(products, i + 1);
	products[i].File := path;
	products[i].Title := title;
	products[i].Parameters := parameters;
	products[i].ForceSuccess := forceSuccess;
	products[i].InstallClean := installClean;
	products[i].MustRebootAfter := mustRebootAfter;
end;

function SmartExec(product : TProduct; var resultcode : Integer): boolean;
{
	Executes a product and returns the exit code.
	Parameters:
		product: the product to install
		resultcode: the exit code
}
begin
	if (LowerCase(Copy(product.File, Length(product.File) - 2, 3)) = 'exe') then begin
		Result := Exec(product.File, product.Parameters, '', SW_SHOWNORMAL, ewWaitUntilTerminated, resultcode);
	end else begin
		Result := ShellExec('', product.File, product.Parameters, '', SW_SHOWNORMAL, ewWaitUntilTerminated, resultcode);
	end;
end;

function PendingReboot: boolean;
{
	Checks whether the machine has a pending reboot.
}
var	names: String;
begin
	if (RegQueryMultiStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Session Manager', 'PendingFileRenameOperations', names)) then begin
		Result := true;
	end else if ((RegQueryMultiStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Session Manager', 'SetupExecute', names)) and (names <> ''))  then begin
		Result := true;
	end else begin
		Result := false;
	end;
end;

function InstallProducts: InstallResult;
{
	Installs the downloaded products
}
var
	resultCode, i, productCount, finishCount: Integer;
begin
	Result := InstallSuccessful;
	productCount := GetArrayLength(products);

	if productCount > 0 then begin
		DependencyPage := CreateOutputProgressPage(CustomMessage('depinstall_title'), CustomMessage('depinstall_description'));
		DependencyPage.Show;

		for i := 0 to productCount - 1 do begin
			if (products[i].InstallClean and (delayedReboot or PendingReboot())) then begin
				Result := InstallRebootRequired;
				break;
			end;

			DependencyPage.SetText(FmtMessage(CustomMessage('depinstall_status'), [products[i].Title]), '');
			DependencyPage.SetProgress(i, productCount);

			while true do begin
				// set 0 as used code for shown error if SmartExec fails
				resultCode := 0;
				if SmartExec(products[i], resultCode) then begin
					// setup executed; resultCode contains the exit code
					if (products[i].MustRebootAfter) then begin
						// delay reboot after install if we installed the last dependency anyways
						if (i = productCount - 1) then begin
							delayedReboot := true;
						end else begin
							Result := InstallRebootRequired;
						end;
						break;
					end else if (resultCode = 0) or (products[i].ForceSuccess) then begin
						finishCount := finishCount + 1;
						break;
					end else if (resultCode = 3010) then begin
						// Windows Installer resultCode 3010: ERROR_SUCCESS_REBOOT_REQUIRED
						delayedReboot := true;
						finishCount := finishCount + 1;
						break;
					end;
				end;

				case MsgBox(FmtMessage(SetupMessage(msgErrorFunctionFailed), [products[i].Title, IntToStr(resultCode)]), mbError, MB_RETRYCANCEL) of
					IDCANCEL: begin
						Result := InstallError;
						break;
					end;
				end;
			end;

			if Result <> InstallSuccessful then begin
				break;
			end;
		end;

		// only leave not installed products for error message
		for i := 0 to productCount - finishCount - 1 do begin
			products[i] := products[i+finishCount];
		end;
		SetArrayLength(products, productCount - finishCount);

		DependencyPage.Hide;
	end;
end;

{
	--------------------
	INNO EVENT FUNCTIONS
	--------------------
}

function PrepareToInstall(var NeedsRestart: boolean): String;
{
	Before the "preparing to install" page.
	See: http://www.jrsoftware.org/ishelp/index.php?topic=scriptevents
}
var
	i: Integer;
	s: string;
begin
	delayedReboot := false;

	case InstallProducts() of
		InstallError: begin
			s := CustomMessage('depinstall_error');

			for i := 0 to GetArrayLength(products) - 1 do begin
				s := s + #13 + '	' + products[i].Title;
			end;

			Result := s;
			end;
		InstallRebootRequired: begin
			Result := products[0].Title;
			NeedsRestart := true;

			// write into the registry that the installer needs to be executed again after restart
			RegWriteStringValue(HKEY_CURRENT_USER, 'SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce', 'InstallBootstrap', ExpandConstant('{srcexe}'));
			end;
	end;
end;

function NeedRestart : boolean;
{
	Checks whether a restart is needed at the end of install
	See: http://www.jrsoftware.org/ishelp/index.php?topic=scriptevents
}
begin
	Result := delayedReboot;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo, MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
{
	Just before the "ready" page.
	See: http://www.jrsoftware.org/ishelp/index.php?topic=scriptevents
}
var
	s: string;
begin
	if downloadMessage <> '' then
		s := s + CustomMessage('depdownload_memo_title') + ':' + NewLine + FmtMessage(downloadMessage, [Space]) + NewLine;
	if installMemo <> '' then
		s := s + CustomMessage('depinstall_memo_title') + ':' + NewLine + FmtMessage(installMemo, [Space]) + NewLine;

	if MemoDirInfo <> '' then
		s := s + MemoDirInfo + NewLine + NewLine;
	if MemoGroupInfo <> '' then
		s := s + MemoGroupInfo + NewLine + NewLine;
	if MemoTasksInfo <> '' then
		s := s + MemoTasksInfo;

	Result := s
end;

function NextButtonClick(CurPageID: Integer): boolean;
{
	At each "next" button click
	See: http://www.jrsoftware.org/ishelp/index.php?topic=scriptevents
}
begin
	Result := true;

	if CurPageID = wpReady then begin
		if downloadMessage <> '' then begin
			if isxdl_DownloadFiles(StrToInt(ExpandConstant('{wizardhwnd}'))) = 0 then
				Result := false;
		end;
	end;
end;
