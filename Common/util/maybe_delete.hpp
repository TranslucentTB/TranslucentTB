#pragma once

namespace Util {
	struct maybe_delete {
	private:
		bool m_ShouldDelete;

	public:
		template<class T>
		void operator()(T *that) const noexcept
		{
			if (m_ShouldDelete)
			{
				delete that;
			}
		}

		maybe_delete(bool shouldDelete) noexcept : m_ShouldDelete(shouldDelete) { }
	};
}
