#pragma once /* logger */
/**
 * @file logger.hpp
 * @brief Simple header-only logging implementation
 */
#if defined(__cplusplus)
#ifndef USE_PCH
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <source_location>
#include <sstream>
#include <unordered_map>
#include <utility>
#endif

#include "core/typedefs.hpp"

using namespace typedefs;
class Logger;

#define LogLevels              \
	X(None, "\033[0m")     \
	X(Trace, "\033[34m")   \
	X(Debug, "\033[35m")   \
	X(Info, "\033[37m")    \
	X(Warning, "\033[33m") \
	X(Error, "\033[31m")   \
	X(Fatal, "\033[0;96")

enum class LogLevel : u8 {
#define X(ID,Color) ID,
	LogLevels
#undef X
};

namespace utils::logger {

/**
 * @brief Describe given error.
 * @tparam E &&err - error value, could be: errno, errc, error_code or system_error.
 * @return A std::string with description.
 */
template <typename E> [[nodiscard]]
inline static std::string explain_err(E &&err)
{
	if constexpr (std::is_same_v<std::remove_cvref_t<E>, std::system_error>) {
		return err.what();
	} else if constexpr (std::is_same_v<std::remove_cvref_t<E>, std::error_code>) {
		return explain_err(std::system_error(err));
	} else if constexpr (std::is_integral_v<std::remove_cvref_t<E>>) {
		return explain_err(std::system_error(static_cast<int>(-err), std::generic_category()));
	} else if constexpr (std::is_same_v<std::remove_cvref_t<E>, std::errc>) {
		return std::make_error_code(err).message();
	} else {
		static_assert(!sizeof(E), "Unsupported type passed to syserr_to_str");
	}
	return "";
}

inline auto todo(std::string &&msg) -> void { std::cout << msg << '\n'; }

inline std::optional<LogLevel> from_string(std::string_view str)
{
#define X(ID, Color)    \
	if (str == #ID) \
		return LogLevel::ID;
	LogLevels
#undef X
	return std::nullopt;
}
} /* namespace utils::logger */

namespace logger::details {

/*  TODO: find working implementation for implicit location passing
 *  except using MACROS for user API
template <typename... Args>
struct fmt_n_loc {
	std::format_string<Args...> fmt_;
	std::source_location        source_;
	template <typename String>
	fmt_n_loc(const String &fmt, std::source_location source = std::source_location::current())
	    : fmt_{fmt},
	      source_{source}
	{ }
};
*/

static inline auto local_time(std::chrono::system_clock::time_point tp)
{
	static const auto* current_tz = std::chrono::current_zone();
	return std::chrono::zoned_time{current_tz, tp};
}

static constexpr std::string_view reset_color = "\033[0m";

static inline std::string to_string(std::source_location const source)
{
	return std::format("{}:[\033[35m{}\033[0m]:{}", std::filesystem::path(source.file_name()).filename().string(),
	                   source.function_name(), source.line());
}

inline std::string to_string(auto tp) { return std::format("{:%F %T %Z}", tp); }

static inline std::string_view to_string(LogLevel lvl)
{
	switch (lvl) {
#define X(ID, Color)       \
	case LogLevel::ID: \
		return #ID;
		LogLevels
#undef X
	}
	return "?";
}

static inline std::string_view get_color(LogLevel lvl)
{
	switch (lvl) {
#define X(ID, Color)       \
	case LogLevel::ID: \
		return Color;
		LogLevels
#undef X
	}
	return "?";
}

} /* namespace logger::details */

namespace logger::sinks {
namespace details {
struct null_mutex {
	static void lock() {}
	static void unlock() {}
};
}

struct Sink {
	virtual ~Sink() = default;

	virtual void write(std::string_view msg) = 0;
	virtual void flush()                     = 0;

	void set_LogLevel(LogLevel level) { level_ = level; }
	[[nodiscard]]
	bool should_log(LogLevel level) const { return level >= level_; }

      protected:
	LogLevel level_{LogLevel::Trace};
};

template <typename Mutex>
struct BaseSink : public Sink {
	[[nodiscard]] BaseSink() = default;
	~BaseSink() override = default;
	BaseSink(const BaseSink &) = delete;
	BaseSink(BaseSink &&) = delete;
	BaseSink &operator=(const BaseSink &) = delete;
	BaseSink &&operator=(BaseSink &&) = delete;

	void write(std::string_view msg) override
	{
		std::lock_guard<Mutex> lock(mutex_);
		write_impl(msg);
	}
	void flush() override
	{
		std::lock_guard<Mutex> lock(mutex_);
		flush_impl();
	}

      protected:
	Mutex mutex_;
	virtual void write_impl(std::string_view msg) = 0;
	virtual void flush_impl()                     = 0;
};

template <typename Mutex>
struct ostreamSink : public BaseSink<Mutex> {
	[[nodiscard]] explicit ostreamSink(std::ostream &os, bool force_flush = false)
	    : ostream_(os),
	      force_flush_(force_flush)
	{ }

	ostreamSink(const ostreamSink &)            = delete;
	ostreamSink &operator=(const ostreamSink &) = delete;

      protected:
	void write_impl(const std::string_view msg) final override
	{
		ostream_.write(msg.data(), static_cast<std::streamsize>(msg.size()));
		if (force_flush_) {
			ostream_.flush();
		}
	}

	void flush_impl() final override { ostream_.flush(); }

	std::ostream &ostream_;
	bool          force_flush_;
};
using ostreamSink_MT = ostreamSink<std::mutex>;
using ostreamSink_ST = ostreamSink<details::null_mutex>;

template <typename Mutex>
struct FileSink : public BaseSink<Mutex> {
	[[nodiscard]] explicit FileSink(const std::filesystem::path &path, bool overwrite = false)
	    : ofs(path, overwrite ? std::ios::trunc : std::ios::app)
	{
		if (!ofs.is_open())
			std::cerr << "Failed to open log file";
	}

	FileSink(const FileSink &)            = delete;
	FileSink &operator=(const FileSink &) = delete;
protected:
	std::ofstream ofs;
	void write_impl(const std::string_view msg) final override { ofs << msg; }
	void flush_impl() final override { ofs.close(); }
};
using FileSink_MT = FileSink<std::mutex>;
using FileSink_ST = FileSink<details::null_mutex>;

} /* namespace logger::sinks */

using sink_ptr = std::shared_ptr<logger::sinks::Sink>;

class Logger {
      public:

	explicit Logger(std::string name, LogLevel level = LogLevel::Error)
	    : level_(level), name_(std::move(name))
	{ }

	template <typename It>
	Logger(std::string name, It begin, It end, LogLevel level = LogLevel::Error)
	    : sinks_(begin, end), level_(level), name_(std::move(name))
	{ }

	Logger(std::string name, sink_ptr sink, LogLevel level = LogLevel::Error)
	    : sinks_{std::move(sink)}, level_(level), name_(std::move(name))
	{ }

	void add_sink(sink_ptr sink) noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		sinks_.push_back(std::move(sink));
	}

	void remove_sink(const sink_ptr &sink) noexcept
	{
		std::lock_guard<std::mutex> lock(mtx_);
		sinks_.erase(std::ranges::find(sinks_,sink));
	}

	void set_LogLevel(LogLevel level) { level_ = level; }
	[[nodiscard]] auto should_log(LogLevel level) const -> bool { return level >= level_; }
	[[nodiscard]] auto name() const -> const std::string & { return name_; }

	template <typename... Args>
	void log(LogLevel level, std::format_string<Args...> fmt, Args &&...args) noexcept
	{
		if (should_log(level))
			log_fmt(level, fmt_rt(fmt, std::forward<Args>(args)...));
	}

	void log(LogLevel lvl, std::string_view msg) noexcept
	{
		if (should_log(lvl))
			log_fmt(lvl, msg);
	}

	struct Stream {
	      private:
		LogLevel                level_;
		/*std::source_location    source_;*/
		std::ostringstream      oss_;
		Logger                 *logger_;

	      public:
		template <typename T> auto operator<<(T &&value) -> Stream &
		{
			oss_ << std::forward<T>(value);
			return *this;
		}

		auto operator<<(std::ostream &(*manip)(std::ostream &)) -> Stream &
		{
			oss_ << manip;
			return *this;
		}

		Stream(Logger *logger, LogLevel lvl /*, std::source_location src = std::source_location::current()*/)
		    : level_(lvl), /*source_(src),*/ logger_(logger)
		{ }

		~Stream() { logger_->log(level_, oss_.str()/*, source_*/); }
	};

	Stream stream(LogLevel level/*, std::source_location source = std::source_location::current()*/)
	{
		return {this, level/*, source*/};
	}

      private:

	void log_fmt(LogLevel level, std::string_view msg/*, std::source_location source = {}*/)
	{
		std::lock_guard<std::mutex> lock(mtx_);

		auto formatted = std::format( "[{}] ({}) {}[{}]{}: {}\n",
			logger::details::to_string(logger::details::local_time(std::chrono::system_clock::now())),
			name_,
			logger::details::get_color(level),
			logger::details::to_string(level),
			logger::details::reset_color,
			msg);

		for (auto &sink : sinks_) {
			if (sink->should_log(level))
				sink->write(formatted);
		}
	}

	template <typename... Args>
	[[nodiscard]] auto fmt_rt(std::format_string<Args...> fmt, Args &&...args) noexcept -> std::string
	{
		std::string msg;
		/* TODO: handle exceptions */
		std::vformat_to(std::back_inserter(msg), fmt.get(), std::make_format_args(args...));
		return msg;
	}

	std::vector<sink_ptr> sinks_;
	LogLevel              level_{LogLevel::Error};
	std::mutex            mtx_;
	std::string           name_;
};

class LoggerRegistry {
      public:
	static LoggerRegistry &inst()
	{
		static LoggerRegistry registry;
		return registry;
	}

	static void register_logger(const std::shared_ptr<Logger> &logger)
	{
		auto &self = inst();
		std::lock_guard<std::mutex> lock(self.mtx_);
		self.loggers_[logger->name()] = logger;
		if (!self.default_logger_)
			self.default_logger_ = logger;
	}

	static std::shared_ptr<Logger> get(std::string_view name)
	{
		auto &self = inst();
		std::lock_guard<std::mutex> lock(self.mtx_);
		auto it = self.loggers_.find(std::string(name));
		return (it != self.loggers_.end()) ? it->second : nullptr;
	}

	static void set_default_logger(std::shared_ptr<Logger> logger)
	{
		auto &self = inst();
		std::lock_guard<std::mutex> lock(self.mtx_);
		self.default_logger_ = std::move(logger);
	}

	static std::shared_ptr<Logger> default_logger()
	{
		auto &self = inst();
		std::lock_guard<std::mutex> lock(self.mtx_);
		if (self.default_logger_)
			return self.default_logger_;
		return std::make_shared<Logger>("default");
	}

	LoggerRegistry(LoggerRegistry &&)                          = delete;
	LoggerRegistry &operator=(LoggerRegistry &&)               = delete;
	LoggerRegistry(const LoggerRegistry &)                     = delete;
	auto operator=(const LoggerRegistry &) -> LoggerRegistry & = delete;
      private:
	LoggerRegistry() = default;

	std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
	std::shared_ptr<Logger>                                  default_logger_;
	std::mutex                                               mtx_;
};

namespace logger::factory {

template <typename Mutex = sinks::details::null_mutex>
inline std::shared_ptr<Logger> stdout_logger(std::string name, std::filesystem::path &path, bool force_flush = false)
{
	auto lg = std::make_shared<Logger>(std::move(name));
	lg->add_sink(std::make_shared<logger::sinks::ostreamSink<Mutex>>(path, force_flush));
	LoggerRegistry::register_logger(lg);
	return lg;
}

template <typename Mutex = sinks::details::null_mutex>
inline std::shared_ptr<Logger> file_logger(std::string name, const std::filesystem::path &file)
{
	auto lg = std::make_shared<Logger>(std::move(name));
	lg->add_sink(std::make_shared<logger::sinks::FileSink<Mutex>>(file));
	LoggerRegistry::register_logger(lg);
	return lg;
}

} /*namespace logger::factory */

namespace logger {

template <typename... Args>
void trace(std::format_string<Args...> fmt, Args &&...args)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Trace, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(std::format_string<Args...> fmt, Args &&...args)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(std::format_string<Args...> fmt, Args &&...args)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Info, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warning(std::format_string<Args...> fmt, Args &&...args)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args &&...args)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Error, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void fatal(std::format_string<Args...> fmt, Args &&...args)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
}

inline void trace(std::string_view msg)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Trace, msg);
}

inline void debug(std::string_view msg)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Debug, msg);
}

inline void info(std::string_view msg)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Info, msg);
}

inline void warning(std::string_view msg)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Warning, msg);
}

inline void error(std::string_view msg)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Error, msg);
}

inline void fatal(std::string_view msg)
{
	auto lg = LoggerRegistry::default_logger();
	lg->log(LogLevel::Fatal, msg);
}

inline auto stream(LogLevel level) -> Logger::Stream
{
	auto lg = LoggerRegistry::default_logger();
	return lg->stream(level);
}

} // namespace logger
#elif !defined(__cplusplus)
/**********************************************************
* Include files
**********************************************************/
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "core/typedefs.hpp"
/**********************************************************
* Macro definitions
**********************************************************/
#define ANSI_RESET "\x1b[0m"
#define ANSI_BLACK "\x1b[90m"
#define ANSI_RED "\x1b[91m"
#define ANSI_GREEN "\x1b[92m"
#define ANSI_YELLOW "\x1b[93m"
#define ANSI_BLUE "\x1b[94m"
#define ANSI_MAGENTA "\x1b[95m"
#define ANSI_CYAN "\x1b[96m"
#define ANSI_WHITE "\x1b[97m"

/**********************************************************
* Variable declarations
**********************************************************/
static const char *level_strings[] = {"EMERGENCY", "ALERT", "CRITICAL", "ERROR",
					"WARNING", "NOTICE", "INFO", "DEBUG"};
typedef enum {
	LOG_LEVEL_EMERGENCY = 0,
	LOG_LEVEL_ALERT = 1,
	LOG_LEVEL_CRITICAL = 2,
	LOG_LEVEL_ERROR = 3,
	LOG_LEVEL_WARNING = 4,
	LOG_LEVEL_NOTICE = 5,
	LOG_LEVEL_INFO = 6,
	LOG_LEVEL_DEBUG = 7,
} log_level;

static log_level current_log_level = LOG_LEVEL_INFO;
static FILE *logfile = NULL;
/**********************************************************
* Functions
**********************************************************/
/**
 * @brief simple wrapper of 'fopen()' to save space in other functions.
 * Opens @filepath FILE with @mode, if @close is greater than 0 - closes file before return.
 * @param filepath - absolute path to file.
 * @param mode - mode to open a file in.
 * @param close - flag which indicates whether to close FILE stream before return.
 * @return returns NULL on errors and sets `errno`, on success returns FILE* file.
 */
static inline FILE *fopen_file(const char *const filepath, const char *const mode, const s32 close)
{
	if (!filepath || !mode) {
		errno = EINVAL;
		return (NULL);
	}

	FILE* file = fopen(filepath, mode);
	if (!file) {
		return (NULL);
	}
	if (close) {
		fclose(file);
	}
	return file;
}

/**
 * @brief Sets log_level to @level and opens @filepath file for logs
 * @param level - log_level enum member (int).
 * @param filepath - full path to file for logs.
 * @return 0 if successfully opened `logfile`, -1 on errors.
 */
static inline int log_init(const log_level level, char *const restrict filepath)
{
	current_log_level = level;
	logfile = (filepath) ? fopen_file(filepath, "a", 0) : stdout;
	return logfile ? (0) : (-1);
}

/**
 * @brief closes @file and sets it's value to NULL
 * @param file - pointer to `FILE`
 */
static inline void log_deinit(void)
{
	fclose(logfile);
	logfile = NULL;
}

/**
 * @brief Returns current time in format: "DayOfTheWeek Month DayOfTheMonth Hours:Minutes:Seconds Year" as a
 * null-terminated string.
 */
static inline char *get_datetime(void)
{
	time_t now = 0;
	time(&now);
	char* date = ctime(&now);
	date[strlen(date) - 1] = '\0';
	return date;
}

/**
 * @brief Logs message to the `logfile`.
 * @param level - level of log to be printed, @see `enum log_level`.
 * @param file - name of the file which called `log_message()`.
 * @param function - name of the function which called `log_message()`.
 * @param line - number of the line on which `log_message()` was called.
 * @param format - format to print @... via `vfprintf()` (%p/%s...).
 * @param ... - arbitrary number of arguments to be passed to `vfprintf()`.
 * @warn suggested usage: `log_message(LOG_LEVEL_DEBUG,__FILE__,__func__,__LINE__, "the value of X is: %d", x);`.
 * @see Macros in `log.h`.
 */
static inline void log_message(const log_level level, const char *const restrict file, const char *const restrict function,
				const int line, const char *format, ...)
{
	if (level > current_log_level) {
		return;
	}
	if (logfile == NULL) {
		logfile = (current_log_level >= LOG_LEVEL_INFO) ? stdout : stderr;
	}

	char *date = get_datetime();
	fprintf(logfile, "%s [%s] %s:%d in %s(): ", date, level_strings[level], file, line, function);

	va_list args;
	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);

	fprintf(logfile, "\n");
	fflush(logfile);
}

#define log_emergency(...) log_message(LOG_LEVEL_EMERGENCY, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define log_alert(...) log_message(LOG_LEVEL_ALERT, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define log_critical(...) log_message(LOG_LEVEL_CRITICAL, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define log_error(...) log_message(LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define log_warning(...) log_message(LOG_LEVEL_WARNING, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define log_notice(...) log_message(LOG_LEVEL_NOTICE, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define log_info(...) log_message(LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_message(LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define bad_args(...) log_message(LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__)
#endif
