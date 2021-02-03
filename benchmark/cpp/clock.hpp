#pragma once

#ifdef _WIN32
#define WIN_CONSOLE_COLOR_GREEN 0x02
#include <windows.h>
#endif

#include <chrono>
#include <iostream>
#include <string>

// TODO: make this class thread safe, to enable parallel benchmarks.
class Clock final {
	using highres_clock = std::chrono::high_resolution_clock;
	using ms = std::chrono::milliseconds;
	using time_point = std::chrono::time_point<highres_clock>;

	static constexpr const char* color_red = "\033[0;32m";
	static constexpr const char* color_reset = "\033[0m";

	// The message that the clock displays
	// upon destruction.
	const std::string_view m_message;
	const time_point m_start_time;
	const std::size_t m_num_runs;

  public:
	explicit Clock(std::string_view&& message, std::size_t ntimes = 1) noexcept
		: m_message{std::move(message)}, m_start_time{highres_clock::now()}, m_num_runs{ntimes} {
	}

	~Clock() {
		const auto dt = highres_clock::now() - m_start_time;
		const auto dt_ms = std::chrono::duration<double>(dt).count() / m_num_runs;

		std::cout << "[";

#ifdef _WIN32
		HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO console_info;
		WORD saved_attrs;

		GetConsoleScreenBufferInfo(h_console, &console_info);
		saved_attrs = console_info.wAttributes;

		SetConsoleTextAttribute(h_console, WIN_CONSOLE_COLOR_GREEN);
#endif
		std::cout << std::fixed << dt_ms << "ms";

#ifdef _WIN32 
		SetConsoleTextAttribute(h_console, saved_attrs);
#endif

		std::cout << "] " << m_message << std::endl;
	}
};
