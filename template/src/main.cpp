/**
  * @file main.cpp
  * @brief Application entry point
  */
/* vim: set noet tw=4 sw=4: */
#ifdef USE_PCH
#include "pch.hpp"
#else
#include "util/logger.hpp"
#endif

int main(void)
{
	#if defined(__cplusplus)
		using namespace logger;
		auto stdout_sink = std::make_shared<sinks::ostreamSink_MT>(std::cout);
		stdout_sink->set_LogLevel(::LogLevel::Info);
		auto logger = std::make_shared<::Logger>("Main Log", stdout_sink);
		::LoggerRegistry::register_logger(logger);
		logger->set_LogLevel(::LogLevel::Info);
		logger::info("Hello world!");
	#elif !defined(__cplusplus)
		log_init(LOG_LEVEL_INFO, NULL);
		log_info("Hello world!\n");
		log_deinit();
	#endif
	return 69;
}

