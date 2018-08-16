[Code]
#ifdef UNICODE
	#define AW "W"
#else
	#define AW "A"
#endif

function MsiEnumRelatedProducts(szUpgradeCode: string; nReserved: dword; nIndex: dword; szProductCode: string): integer;
external 'MsiEnumRelatedProducts{#AW}@msi.dll stdcall';

function MsiGetProductInfo(szProductCode: string; szProperty: string; szValue: string; var nvalueSize: dword): integer;
external 'MsiGetProductInfo{#AW}@msi.dll stdcall';

function msiproductupgrade(upgradeCode: string; minVersion: string): boolean;
var
	productCode, version: string;
	valueSize: dword;
begin
	SetLength(productCode, 39);
	Result := false;

	if (MsiEnumRelatedProducts(upgradeCode, 0, 0, productCode) = 0) then begin
		SetLength(version, 39);
		valueSize := Length(version);

		if (MsiGetProductInfo(productCode, 'VersionString', version, valueSize) = 0) then begin
			Result := compareversion(version, minVersion) >= 0;
		end;
	end;
end;

[Setup]
