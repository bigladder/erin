// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "erin/network-utils.h"

TEST(NetworkUtils, test_no_strongly_connected_components_for_DAG)
{
    std::vector<std::string> nodes = {
        "A",
        "B",
        "C",
    };
    std::vector<std::pair<size_t, size_t>> edges = {
        {0, 1},
        {1, 2},
    };
    std::vector<std::vector<std::string>> sccs =
        erin::find_strongly_connected_components(nodes, edges);
    EXPECT_EQ(sccs.size(), 0);
}

TEST(NetworkUtils, test_find_1_strongly_connected_component_set)
{
    std::vector<std::string> nodes = {
        "A",
        "B",
        "C",
    };
    std::vector<std::pair<size_t, size_t>> edges = {
        {0, 1},
        {1, 2},
        {2, 0},
    };
    std::vector<std::vector<std::string>> sccs =
        erin::find_strongly_connected_components(nodes, edges);
    EXPECT_EQ(sccs.size(), 1);
    EXPECT_EQ(sccs[0].size(), 3);
    bool found_A = false;
    bool found_B = false;
    bool found_C = false;
    for (std::string const& node : sccs[0])
    {
        if (node == "A")
        {
            found_A = true;
        }
        else if (node == "B")
        {
            found_B = true;
        }
        else if (node == "C")
        {
            found_C = true;
        }
        else
        {
            FAIL() << "Didn't find 'A', 'B', or 'C'";
        }
    }
    EXPECT_TRUE(found_A);
    EXPECT_TRUE(found_B);
    EXPECT_TRUE(found_C);
}
