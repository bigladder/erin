// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include "erin/logging.h"

namespace erin
{
unsigned int LogLevel_ToInt(LogLevel ll)
{
    switch (ll)
    {
    case LogLevel::debug:
        return 0;
    case LogLevel::info:
        return 1;
    case LogLevel::warning:
        return 2;
    case LogLevel::error:
        return 3;
    }
    std::cout << "Error: unhandled log level" << std::endl;
    std::exit(1);
}

bool ContinueLogging(LogLevel incoming, LogLevel reference)
{
    return LogLevel_ToInt(incoming) >= LogLevel_ToInt(reference);
}

Log Log_make_from_courier(Courier::Courier& courier)
{
    return Log {
        .debug =
            [&](std::string const& tag, std::string const& msg)
        {
            if (!tag.empty())
            {
                courier.send_debug(fmt::format("{}: {}", tag, msg));
            }
            else
            {
                courier.send_debug(msg);
            }
        },
        .info =
            [&](std::string const& tag, std::string const& msg)
        {
            if (!tag.empty())
            {
                courier.send_info(fmt::format("{}: {}", tag, msg));
            }
            else
            {
                courier.send_info(msg);
            }
        },
        .warning =
            [&](std::string const& tag, std::string const& msg)
        {
            if (!tag.empty())
            {
                courier.send_warning(fmt::format("{}: {}", tag, msg));
            }
            else
            {
                courier.send_warning(msg);
            }
        },
        .error =
            [&](std::string const& tag, std::string const& msg)
        {
            if (!tag.empty())
            {
                courier.send_error(fmt::format("{}: {}", tag, msg));
            }
            else
            {
                courier.send_error(msg);
            }
        },
    };
}

void Log_general(Log const& log, LogLevel ll, std::string const& msg)
{
    Log_general(log, ll, "", msg);
}

void Log_general(Log const& log, LogLevel ll, std::string const& tag, std::string const& msg)
{
    if (!ContinueLogging(ll, log.log_level))
    {
        return;
    }
    std::optional<std::function<void(std::string const&, std::string const&)>> maybeFn = {};
    switch (ll)
    {
    case LogLevel::debug:
    {
        maybeFn = log.debug;
    }
    break;
    case LogLevel::info:
    {
        maybeFn = log.info;
    }
    break;
    case LogLevel::warning:
    {
        maybeFn = log.warning;
    }
    break;
    case LogLevel::error:
    {
        maybeFn = log.error;
    }
    break;
    default:
    {
        std::cout << "Unhandled logging level" << std::endl;
        std::exit(1);
    }
    }
    if (maybeFn.has_value())
    {
        std::function<void(std::string const&, std::string const&)> f = maybeFn.value();
        f(tag, msg);
    }
}

void Log_debug(Log const& log, std::string const& msg) { Log_general(log, LogLevel::debug, msg); }

void Log_debug(Log const& log, std::string const& tag, std::string const& msg)
{
    Log_general(log, LogLevel::debug, tag, msg);
}

void Log_info(Log const& log, std::string const& msg) { Log_general(log, LogLevel::info, msg); }

void Log_info(Log const& log, std::string const& tag, std::string const& msg)
{
    Log_general(log, LogLevel::info, tag, msg);
}

void Log_warning(Log const& log, std::string const& msg)
{
    Log_general(log, LogLevel::warning, msg);
}

void Log_warning(Log const& log, std::string const& tag, std::string const& msg)
{
    Log_general(log, LogLevel::warning, tag, msg);
}

void Log_error(Log const& log, std::string const& msg) { Log_general(log, LogLevel::error, msg); }

void Log_error(Log const& log, std::string const& tag, std::string const& msg)
{
    Log_general(log, LogLevel::error, tag, msg);
}
} // namespace erin
