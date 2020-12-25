#pragma once
#include "arch.h"
#include <filesystem>
#include <mutex>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <string_view>
#include <type_traits>
#include <wil/resource.h>

enum class lazy_sink_state {
	opened,
	nothing_logged,
	failed
};

template<typename Mutex>
class lazy_file_sink final : public spdlog::sinks::base_sink<Mutex> {
	using path_getter_t = std::add_pointer_t<std::filesystem::path()>;

public:
	explicit lazy_file_sink(std::filesystem::path path) : m_File(std::move(path)), m_Tried(false) { }

	const std::filesystem::path &file() const noexcept { return m_File; }

	lazy_sink_state state() const noexcept
	{
		if (m_Tried)
		{
			return m_Handle
				? lazy_sink_state::opened
				: lazy_sink_state::failed;
		}

		return lazy_sink_state::nothing_logged;
	}

protected:
	void sink_it_(const spdlog::details::log_msg &msg) override;
	void flush_() override;

private:
	bool m_Tried;
	wil::unique_hfile m_Handle;
	std::filesystem::path m_File;

	void open();

	template<typename T>
	void write(const T &thing);
};

using lazy_file_sink_mt = lazy_file_sink<std::mutex>;
using lazy_file_sink_st = lazy_file_sink<spdlog::details::null_mutex>;
