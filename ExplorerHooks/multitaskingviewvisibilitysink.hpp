#pragma once
#include "arch.h"
#include <type_traits>
#include <wrl/implements.h>

#include "constants.hpp"
#include "timelinevisibilitymonitor.hpp"
#include "undoc/explorer.hpp"

class MultitaskingViewVisibilitySink : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IMultitaskingViewVisibilityNotification> {
	void NotifyWorker(bool opened) const noexcept
	{
		if (const auto worker = FindWindow(WORKER_WINDOW.c_str(), WORKER_WINDOW.c_str()))
		{
			// avoid freezing Explorer if our main process is frozen
			SendMessageTimeout(worker, TimelineVisibilityMonitor::s_TimelineNotification, opened, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 50, nullptr);
		}
	}

	IFACEMETHODIMP MultitaskingViewShown(MULTITASKING_VIEW_TYPES flags) noexcept override
	{
		if (flags & MVT_ALL_UP_VIEW)
		{
			NotifyWorker(true);
		}

		return S_OK;
	}

	IFACEMETHODIMP MultitaskingViewDismissed(MULTITASKING_VIEW_TYPES flags) noexcept override
	{
		if (flags & MVT_ALL_UP_VIEW)
		{
			NotifyWorker(false);
		}

		return S_OK;
	}
};
