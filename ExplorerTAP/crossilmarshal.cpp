#include "crossilmarshal.hpp"

#include <objidl.h>
#include <wil/com.h>
#include <wil/resource.h>

#include <cstring>

#include "ITaskbarAppearanceService.h"

namespace
{
	using namespace std::string_view_literals;

	constexpr std::wstring_view MAPPING_NAME = L"Local\\TTB_TAPService_Marshal"sv;
	constexpr DWORD MAPPING_SIZE = 64 * 1024;
	constexpr DWORD HEADER_MAGIC = 0x54415053; // 'TAPS'

	struct Header
	{
		DWORD magic;
		DWORD dataSize;
		DWORD ownerProcessId;
		DWORD reserved;
	};
}

HRESULT CrossILMarshal::Publish(::IUnknown* service, DWORD ownerProcessId) noexcept
try
{
	if (!service)
	{
		return E_INVALIDARG;
	}

	// Marshal to a stream first: it produces the OBJREF blob we can copy into shared memory.
	wil::com_ptr<IStream> stream;
	RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.put()));
	RETURN_IF_FAILED(CoMarshalInterface(stream.get(), IID_ITaskbarAppearanceService, service, MSHCTX_LOCAL, nullptr, MSHLFLAGS_TABLESTRONG));

	HGLOBAL global = nullptr;
	RETURN_IF_FAILED(GetHGlobalFromStream(stream.get(), &global));

	const SIZE_T dataSize = GlobalSize(global);
	if (dataSize == 0 || dataSize > MAPPING_SIZE - sizeof(Header))
	{
		return E_UNEXPECTED;
	}

	void* source = GlobalLock(global);
	if (!source)
	{
		return E_UNEXPECTED;
	}

	// Static so the mapping outlives this call - it must stay available for as long as
	// the service exists, i.e. until Explorer unloads this DLL.
	static wil::unique_handle mapping(CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, MAPPING_SIZE, MAPPING_NAME.data()));
	if (!mapping)
	{
		GlobalUnlock(global);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	void* view = MapViewOfFile(mapping.get(), FILE_MAP_WRITE, 0, 0, 0);
	if (!view)
	{
		GlobalUnlock(global);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	const Header header { HEADER_MAGIC, static_cast<DWORD>(dataSize), ownerProcessId, 0 };
	std::memcpy(view, &header, sizeof(header));
	std::memcpy(static_cast<BYTE*>(view) + sizeof(Header), source, dataSize);

	UnmapViewOfFile(view);
	GlobalUnlock(global);
	return S_OK;
}
catch (...)
{
	return winrt::to_hresult();
}

winrt::com_ptr<::IUnknown> CrossILMarshal::Retrieve(DWORD expectedProcessId) noexcept
try
{
	winrt::com_ptr<::IUnknown> result;

	wil::unique_handle mapping(OpenFileMappingW(FILE_MAP_READ, FALSE, MAPPING_NAME.data()));
	if (!mapping)
	{
		return result;
	}

	void* view = MapViewOfFile(mapping.get(), FILE_MAP_READ, 0, 0, 0);
	if (!view)
	{
		return result;
	}

	Header header { };
	std::memcpy(&header, view, sizeof(header));
	const bool valid = header.magic == HEADER_MAGIC &&
		header.dataSize != 0 &&
		header.dataSize <= MAPPING_SIZE - sizeof(Header) &&
		header.ownerProcessId == expectedProcessId;

	if (!valid)
	{
		UnmapViewOfFile(view);
		return result;
	}

	wil::com_ptr<IStream> stream;
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.put())))
	{
		UnmapViewOfFile(view);
		return result;
	}

	if (FAILED(stream->Write(static_cast<BYTE*>(view) + sizeof(Header), header.dataSize, nullptr)))
	{
		UnmapViewOfFile(view);
		return result;
	}

	const LARGE_INTEGER zero { };
	if (FAILED(stream->Seek(zero, STREAM_SEEK_SET, nullptr)))
	{
		UnmapViewOfFile(view);
		return result;
	}

	// Requires the proxy/stub, which the caller installs before attempting this.
	CoUnmarshalInterface(stream.get(), IID_ITaskbarAppearanceService, result.put_void());

	UnmapViewOfFile(view);
	return result;
}
catch (...)
{
	return { };
}
