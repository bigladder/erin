/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_LOGGING_H
#define ERIN_LOGGING_H
#include <functional>
#include <string>
#include <optional>
#include <iostream>
#include <cstdlib>
#include "../vendor/courier/include/courier/courier.h"
#include "../vendor/fmt/include/fmt/core.h"
#include "erin_next/erin_next_utils.h"

namespace erin
{
    enum class LogLevel
    {
        Debug = 0,
        Info,
        Warning,
        Error,
    };

    struct Log
    {
        LogLevel LogLevel = LogLevel::Debug;
        std::optional<
            std::function<void(std::string const&, std::string const&)>>
            debug = {};
        std::optional<
            std::function<void(std::string const&, std::string const&)>>
            info = {};
        std::optional<
            std::function<void(std::string const&, std::string const&)>>
            warning = {};
        std::optional<
            std::function<void(std::string const&, std::string const&)>>
            error = {};
    };

    void
    Log_General(Log const& log, LogLevel ll, std::string const& msg);

    void
    Log_General(
        Log const& log,
        LogLevel ll,
        std::string const& tag,
        std::string const& msg
    );

    void
    Log_Debug(Log const& log, std::string const& msg);

    void
    Log_Debug(Log const& log, std::string const& tag, std::string const& msg);

    void
    Log_Info(Log const& log, std::string const& msg);

    void
    Log_Info(Log const& log, std::string const& tag, std::string const& msg);

    void
    Log_Warning(Log const& log, std::string const& msg);

    void
    Log_Warning(Log const& log, std::string const& tag, std::string const& msg);

    void
    Log_Error(Log const& log, std::string const& msg);

    void
    Log_Error(Log const& log, std::string const& tag, std::string const& msg);

    class Logger final: public Courier::Courier
    {
      public:
        void
        receive_error(const std::string& message) override
        {
            write_message("ERROR", message);
        }

        void
        receive_warning(const std::string& message) override
        {
            write_message("WARNING", message);
        }

        void
        receive_info(const std::string& message) override
        {
            write_message("INFO", message);
        }

        void
        receive_debug(const std::string& message) override
        {
            write_message("DEBUG", message);
        }

        static void
        write_message(
            const std::string& message_type,
            const std::string& message
        )

        {
            std::cout << fmt::format("[{}] {}", message_type, message)
                      << std::endl;
        }
    };

    Log
    Log_MakeFromCourier(Courier::Courier& courier);
}

#endif
