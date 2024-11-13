// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <exception>

#include <gtest/gtest.h>

#include "erin/logging.h"

using namespace erin;

TEST(Logging, TestWeCanLog)
{
    Logger logger {};
    Log log = Log_make_from_courier(logger);
    Log_debug(log, "this is a debug statement");
    Log_info(log, "this is an info statement");
    Log_warning(log, "this is a warning statement");
    EXPECT_THROW(Log_error(log, "this is an error -- using courier, it throws..."), std::exception);
}
