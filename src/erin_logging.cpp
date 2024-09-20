/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/logging.h"

namespace erin
{
    unsigned int
    LogLevel_ToInt(LogLevel ll)
    {
        switch (ll)
        {
            case LogLevel::Debug:
                return 0;
            case LogLevel::Info:
                return 1;
            case LogLevel::Warning:
                return 2;
            case LogLevel::Error:
                return 3;
        }
        std::cout << "Error: unhandled log level" << std::endl;
        std::exit(1);
    }

    bool
    ContinueLogging(LogLevel incoming, LogLevel reference)
    {
        return LogLevel_ToInt(incoming) >= LogLevel_ToInt(reference);
    }

    Log
    Log_MakeFromCourier(Courier::Courier& courier)
    {
        return Log{
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

    void
    Log_General(Log const& log, LogLevel ll, std::string const& msg)
    {
        Log_General(log, ll, "", msg);
    }

    void
    Log_General(
        Log const& log,
        LogLevel ll,
        std::string const& tag,
        std::string const& msg
    )
    {
        if (!ContinueLogging(ll, log.LogLevel))
        {
            return;
        }
        std::optional<
            std::function<void(std::string const&, std::string const&)>>
            maybeFn = {};
        switch (ll)
        {
            case LogLevel::Debug:
            {
                maybeFn = log.debug;
            }
            break;
            case LogLevel::Info:
            {
                maybeFn = log.info;
            }
            break;
            case LogLevel::Warning:
            {
                maybeFn = log.warning;
            }
            break;
            case LogLevel::Error:
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
            std::function<void(std::string const&, std::string const&)> f =
                maybeFn.value();
            f(tag, msg);
        }
    }

    void
    Log_Debug(Log const& log, std::string const& msg)
    {
        Log_General(log, LogLevel::Debug, msg);
    }

    void
    Log_Debug(Log const& log, std::string const& tag, std::string const& msg)
    {
        Log_General(log, LogLevel::Debug, tag, msg);
    }

    void
    Log_Info(Log const& log, std::string const& msg)
    {
        Log_General(log, LogLevel::Info, msg);
    }

    void
    Log_Info(Log const& log, std::string const& tag, std::string const& msg)
    {
        Log_General(log, LogLevel::Info, tag, msg);
    }

    void
    Log_Warning(Log const& log, std::string const& msg)
    {
        Log_General(log, LogLevel::Warning, msg);
    }

    void
    Log_Warning(Log const& log, std::string const& tag, std::string const& msg)
    {
        Log_General(log, LogLevel::Warning, tag, msg);
    }

    void
    Log_Error(Log const& log, std::string const& msg)
    {
        Log_General(log, LogLevel::Error, msg);
    }

    void
    Log_Error(Log const& log, std::string const& tag, std::string const& msg)
    {
        Log_General(log, LogLevel::Error, tag, msg);
    }
}
