/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/logging.h"
#include <exception>
#include <gtest/gtest.h>

using namespace erin;

TEST(Logging, TestWeCanLog)
{
    Logger logger{};
    Log log = Log_MakeFromCourier(logger);
    Log_Debug(log, "this is a debug statement");
    Log_Info(log, "this is an info statement");
    Log_Warning(log, "this is a warning statement");
    EXPECT_THROW(
        Log_Error(log, "this is an error -- using courier, it throws..."),
        std::exception
    );
}
