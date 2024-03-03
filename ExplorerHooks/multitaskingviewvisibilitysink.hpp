#pragma once
#include "arch.h"
#include <type_traits>
#include <wrl/implements.h>

#include "constants.hpp"
#include "taskviewvisibilitymonitor.hpp"
#include "undoc/explorer.hpp"

class MultitaskingViewVisibilitySink : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IMultitaskingViewVisibilityNotification> {
private:
	UINT m_ChangeMessage;

	void NotifyWorker(bool opened) const noexcept
	{
		if (const auto worker = FindWindow(TTB_WORKERWINDOW.c_str(), TTB_WORKERWINDOW.c_str()))
		{
			PostMessage(worker, m_ChangeMessage, opened, 0);
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

public:
	MultitaskingViewVisibilitySink(UINT changeMessage) noexcept : m_ChangeMessage(changeMessage) { }
};
