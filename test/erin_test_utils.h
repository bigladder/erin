/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_TEST_UTILS_H
#define ERIN_TEST_UTILS_H
#include "gtest/gtest.h"
#include <iostream>
#include <unordered_map>


namespace erin_test_utils
{
  const double tolerance{1e-6};

  template <class T>
  void compare_maps(
    const std::unordered_map<std::string, T>& expected,
    const std::unordered_map<std::string, T>& actual,
    const std::string& tag)
  {
    EXPECT_EQ(expected.size(), actual.size());
    for (const auto& e: expected) {
      auto a_it = actual.find(e.first);
      ASSERT_FALSE(a_it == actual.end());
      auto a_val = a_it->second;
      auto e_val = e.second;
      EXPECT_NEAR(
        static_cast<double>(e_val),
        static_cast<double>(a_val),
        tolerance) << "tag: " << tag << "; key: " << e.first;
    }
  }

  template <class T>
  void compare_maps_exact(
    const std::unordered_map<std::string, T>& expected,
    const std::unordered_map<std::string, T>& actual,
    const std::string& tag)
  {
    EXPECT_EQ(expected.size(), actual.size());
    for (const auto& e : expected) {
      auto a_it = actual.find(e.first);
      ASSERT_FALSE(a_it == actual.end());
      auto a_val = a_it->second;
      auto e_val = e.second;
      EXPECT_EQ(e_val, a_val)
        << "tag: " << tag << "; "
        << "key: " << e.first;
    }
  }

  template <class T>
  void compare_vectors(
      const std::vector<T>& expected,
      const std::vector<T>& actual)
  {
    auto expected_size{expected.size()};
    ASSERT_EQ(expected_size, actual.size());
    for (decltype(expected_size) i{0}; i < expected_size; ++i)
      EXPECT_EQ(expected[i], actual[i]) << "vectors differ at index " << i;
  }

  template <class T>
  bool compare_vectors_functional(
      const std::vector<T>& expected,
      const std::vector<T>& actual)
  {
    auto expected_size{expected.size()};
    if (expected_size != actual.size()) {
      std::cout << "expected_size != actual.size()\n"
                << "expected_size = " << expected_size << "\n"
                << "actual.size() = " << actual.size() << "\n";
      return false;
    }
    for (decltype(expected_size) i{0}; i < expected_size; ++i) {
      if (expected[i] != actual[i]) {
        std::cout << "expected[" << i << "] != actual[" << i << "]\n"
                  << "expected[" << i << "] = " << expected[i] << "\n"
                  << "actual[" << i << "] = " << actual[i] << "\n";
        return false;
      }
    }
    return true;
  }
}

#endif // ERIN_TEST_UTILS_H
