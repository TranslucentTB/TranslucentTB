#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <wil/safecast.h>
#include <winrt/base.h>
#include <winstring.h>

namespace Util {
	class hstring_builder {
	private:
		HSTRING_BUFFER m_Handle;
		wchar_t *m_Buf;
		uint32_t m_Capacity, m_Size;

	public:
		using value_type = wchar_t;

		hstring_builder(uint32_t capacity) : m_Handle(nullptr), m_Buf(nullptr), m_Capacity(capacity), m_Size(0)
		{
			winrt::check_hresult(WindowsPreallocateStringBuffer(capacity, &m_Buf, &m_Handle));
		}

		hstring_builder(const hstring_builder &) = delete;
		hstring_builder &operator =(const hstring_builder &) = delete;

		void push_back(wchar_t ch)
		{
			if (m_Buf)
			{
				if (m_Size + 1 <= m_Capacity)
				{
					m_Buf[m_Size++] = ch;
				}
				else
				{
					throw std::length_error("HSTRING buffer too small");
				}
			}
			else
			{
				throw std::runtime_error("String has already been finalized");
			}
		}

		void append(const wchar_t *s, uint32_t n)
		{
			if (m_Buf)
			{
				if (m_Size + n <= m_Capacity)
				{
					memcpy(m_Buf + m_Size, s, n * sizeof(wchar_t));
					m_Size += n;
				}
				else
				{
					throw std::length_error("HSTRING buffer too small");
				}
			}
			else
			{
				throw std::runtime_error("String has already been finalized");
			}
		}

		void append(std::wstring_view str)
		{
			append(str.data(), wil::safe_cast<uint32_t>(str.size()));
		}

		winrt::hstring to_hstring()
		{
			if (m_Size == m_Capacity)
			{
				winrt::hstring hstr;
				winrt::check_hresult(WindowsPromoteStringBuffer(m_Handle, reinterpret_cast<HSTRING *>(winrt::put_abi(hstr))));
				m_Handle = nullptr;
				m_Buf = nullptr;
				return hstr;
			}
			else
			{
				throw std::runtime_error("HSTRING buffer not full");
			}
		}

		~hstring_builder()
		{
			if (m_Handle)
			{
				winrt::check_hresult(WindowsDeleteStringBuffer(m_Handle));
			}
		}
	};
}
