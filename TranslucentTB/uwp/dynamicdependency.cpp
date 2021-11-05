#include "dynamicdependency.hpp"
#include "../ProgramLog/error/win32.hpp"

DynamicDependency::DynamicDependency(Util::null_terminated_wstring_view packageFamilyName, const PACKAGE_VERSION &minVersion, bool hasPackageIdentity) :
	m_Context(nullptr)
{
	if (!hasPackageIdentity)
	{
		if (!IsApiSetImplemented("api-ms-win-appmodel-runtime-l1-1-5"))
		{
			MessagePrint(spdlog::level::critical, L"Portable mode only works on Windows 11");
		}

		static constexpr PackageDependencyProcessorArchitectures arch =
#if defined(_M_AMD64)
			PackageDependencyProcessorArchitectures_X64;
#elif defined(_M_ARM64)
			PackageDependencyProcessorArchitectures_Arm64;
#endif

		// we are using process dependency lifetime because we want the app to be portable, so we cannot be sure if a dependency ID created by a
		// previous instance is valid anymore - the app might have been launched on another computer. Plus I'm too lazy to implement proper storage.
		HRESULT hr = TryCreatePackageDependency(nullptr, packageFamilyName.c_str(), minVersion, arch, PackageDependencyLifetimeKind_Process, nullptr, CreatePackageDependencyOptions_None, m_dependencyId.put());
		if (FAILED(hr)) [[unlikely]]
		{
			fmt::wmemory_buffer buf;
			fmt::format_to(buf, FMT_STRING(L"Failed to create a dynamic dependency for {} version {}.{}.{}.{} - perhaps it is not installed?"), packageFamilyName, minVersion.Major, minVersion.Minor, minVersion.Build, minVersion.Revision);
			HresultHandle(hr, spdlog::level::critical, buf);
		}

		wil::unique_process_heap_string packageFullName;
		hr = AddPackageDependency(m_dependencyId.get(), 0, AddPackageDependencyOptions_None, &m_Context, packageFullName.put());
		if (FAILED(hr)) [[unlikely]]
		{
			HresultHandle(hr, spdlog::level::critical, L"Failed to add a runtime dependency");
		}
	}
}

DynamicDependency::~DynamicDependency() noexcept(false)
{
	if (m_Context)
	{
		HRESULT hr = RemovePackageDependency(m_Context);
		if (SUCCEEDED(hr))
		{
			m_Context = nullptr;
		}
		else
		{
			// we get random invalid parameter errors despite the parameter being correct.
			HresultHandle(hr, spdlog::level::info, L"Failed to remove a runtime dependency");
		}

		hr = DeletePackageDependency(m_dependencyId.get());
		if (FAILED(hr)) [[unlikely]]
		{
			HresultHandle(hr, spdlog::level::warn, L"Failed to delete a dynamic dependency");
		}
	}
}
