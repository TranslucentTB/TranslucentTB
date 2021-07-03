#pragma once
#include "arch.h"
#include <type_traits>
#include <wrl/implements.h>

#include "constants.hpp"
#include "taskviewvisibilitymonitor.hpp"
#include "undoc/explorer.hpp"

class MultitaskingViewVisibilitySink : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IMultitaskingViewVisibilityNotification> {
	void NotifyWorker(bool opened) const noexcept
	{
		if (const auto worker = FindWindow(TTB_WORKERWINDOW.c_str(), TTB_WORKERWINDOW.c_str()))
		{
			// avoid freezing Explorer if our main process is frozen
			SendMessageTimeout(worker, TaskViewVisibilityMonitor::s_TaskViewVisibilityChangeMessage, opened, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 50, nullptr);
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
