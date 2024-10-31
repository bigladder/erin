// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#ifndef ERIN_LOGGING_H
#define ERIN_LOGGING_H
#include <functional>
#include <string>
#include <optional>
#include <iostream>
#include <cstdlib>
#include "../vendor/courier/include/courier/courier.h"
#include "../vendor/fmt/include/fmt/core.h"
#include "erin/utils.h"

namespace erin
{
enum class LogLevel
{
    debug = 0,
    info,
    warning,
    error,
};

struct Log
{
    LogLevel log_level = LogLevel::debug;
    std::optional<std::function<void(std::string const&, std::string const&)>> debug = {};
    std::optional<std::function<void(std::string const&, std::string const&)>> info = {};
    std::optional<std::function<void(std::string const&, std::string const&)>> warning = {};
    std::optional<std::function<void(std::string const&, std::string const&)>> error = {};
};

void Log_general(Log const& log, LogLevel ll, std::string const& msg);

void Log_general(Log const& log, LogLevel ll, std::string const& tag, std::string const& msg);

void Log_debug(Log const& log, std::string const& msg);

void Log_debug(Log const& log, std::string const& tag, std::string const& msg);

void Log_info(Log const& log, std::string const& msg);

void Log_info(Log const& log, std::string const& tag, std::string const& msg);

void Log_warning(Log const& log, std::string const& msg);

void Log_warning(Log const& log, std::string const& tag, std::string const& msg);

void Log_error(Log const& log, std::string const& msg);

void Log_error(Log const& log, std::string const& tag, std::string const& msg);

class Logger final : public Courier::Courier
{
  public:
    void receive_error(const std::string& message) override { write_message("ERROR", message); }

    void receive_warning(const std::string& message) override { write_message("WARNING", message); }

    void receive_info(const std::string& message) override { write_message("INFO", message); }

    void receive_debug(const std::string& message) override { write_message("DEBUG", message); }

    static void write_message(const std::string& message_type, const std::string& message)

    {
        std::cout << fmt::format("[{}] {}", message_type, message) << std::endl;
    }
};

Log Log_make_from_courier(Courier::Courier& courier);
} // namespace erin

#endif
