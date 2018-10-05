template<bool newValue = true>
class bool_guard {
private:
	bool m_oldValue;
	bool &m_boolRef;

public:
	inline explicit bool_guard(bool &boolRef) : m_oldValue(boolRef), m_boolRef(boolRef)
	{
		m_boolRef = newValue;
	}

	inline bool_guard(const bool_guard &) = delete;
	inline bool_guard &operator =(const bool_guard &) = delete;

	inline ~bool_guard()
	{
		m_boolRef = m_oldValue;
	}
};