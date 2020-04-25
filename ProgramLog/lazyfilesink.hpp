#pragma once
#include "arch.h"
#include <filesystem>
#include <mutex>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#include <string_view>
#include <type_traits>
#include <wil/resource.h>

template<typename Mutex>
class lazy_file_sink final : public spdlog::sinks::base_sink<Mutex> {
	using path_getter_t = std::add_pointer_t<std::filesystem::path()>;
public:
	explicit lazy_file_sink(path_getter_t getter) : m_PathGetter(getter), m_Tried(false) { }

	const std::filesystem::path &file() const noexcept { return m_File; }
	bool opened() const noexcept { return m_Handle.is_valid(); }
	bool tried() const noexcept { return m_Tried; }

protected:
	void sink_it_(const spdlog::details::log_msg &msg) override;
	void flush_() override;

private:
	path_getter_t m_PathGetter;
	bool m_Tried;
	wil::unique_hfile m_Handle;
	std::filesystem::path m_File;

	void open();
	void handle_open_error(std::wstring_view err);

	template<typename T>
	void write(const T &thing);
};

using lazy_file_sink_mt = lazy_file_sink<std::mutex>;
using lazy_file_sink_st = lazy_file_sink<spdlog::details::null_mutex>;
