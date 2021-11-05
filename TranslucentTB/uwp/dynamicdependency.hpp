#pragma once
#include "arch.h"
#include <windef.h>
#include <WinBase.h>
#include <appmodel.h>
#include <wil/resource.h>

#include "util/null_terminated_string_view.hpp"

class DynamicDependency {
	wil::unique_process_heap_string m_dependencyId;
	PACKAGEDEPENDENCY_CONTEXT m_Context;

public:
	DynamicDependency(Util::null_terminated_wstring_view packageFamilyName, const PACKAGE_VERSION &minVersion, bool hasPackageIdentity);

	DynamicDependency(const DynamicDependency &) = delete;
	DynamicDependency &operator =(const DynamicDependency &) = delete;

	~DynamicDependency() noexcept(false);
};
