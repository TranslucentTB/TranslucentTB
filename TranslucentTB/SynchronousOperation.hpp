#pragma once
#ifdef STORE

#include <Windows.Foundation.Collections.h>
#include <wrl/implements.h>
#include <wrl/event.h>

// Found in libil2cpp's headers
template <typename T>
class SynchronousOperation : Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::WinRtClassicComMix>, ABI::Windows::Foundation::IAsyncOperationCompletedHandler<T>>
{
private:
	Microsoft::WRL::Wrappers::Event m_Event;
	HRESULT m_HR;
	T m_Result;

public:
	inline SynchronousOperation(ABI::Windows::Foundation::IAsyncOperation<T> *op)
	{
		m_Event.Attach(CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, WRITE_OWNER | EVENT_ALL_ACCESS));

		op->put_Completed(this);
	}

	HRESULT GetResults(T *result)
	{
		DWORD waitResult = WaitForSingleObject(m_Event.Get(), INFINITE);

		if (waitResult != WAIT_OBJECT_0)
		{
			if (waitResult == WAIT_FAILED)
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}
			else
			{
				return E_FAIL;
			}
		}

		if (FAILED(m_HR))
		{
			return m_HR;
		}

		*result = m_Result;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Invoke(ABI::Windows::Foundation::IAsyncOperation<T> *asyncInfo, ABI::Windows::Foundation::AsyncStatus) override
	{
		m_HR = asyncInfo->GetResults(&m_Result);
		SetEvent(m_Event.Get());
		return S_OK;
	}
};

#endif // STORE