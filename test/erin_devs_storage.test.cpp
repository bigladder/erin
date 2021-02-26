/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/devs.h"
#include "erin/devs/storage.h"
#include "erin/devs/runner.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

namespace ED = erin::devs;

bool
test_undisturbed_discharge(bool show_details)
{
  using size_type = std::vector<double>::size_type;
  const std::vector<ED::RealTimeType> expected_times_s{0, 0, 0, 20, 40};
  const std::vector<ED::FlowValueType> expected_socs{1.0, 1.0, 1.0, 0.0, 1.0};
  const std::vector<ED::FlowValueType> expected_inflows_requested{0.0, 5.0, 5.0, 10.0, 5.0};
  const std::vector<ED::FlowValueType> expected_inflows_achieved{0.0, 5.0, 0.0, 10.0, 5.0};
  const std::vector<ED::FlowValueType> expected_outflows_requested{0.0, 5.0, 5.0, 5.0, 5.0};
  const std::vector<ED::FlowValueType> expected_outflows_achieved{0.0, 5.0, 5.0, 5.0, 5.0};
  const ED::FlowValueType tol{1e-6};
  auto data = ED::storage_make_data(100.0, 10.0);
  auto s0 = ED::storage_make_state(data, 1.0);
  const std::vector<ED::RealTimeType> times_s{
    0,
    0,
  };
  const std::vector<std::vector<ED::PortValue>> xss{
    {ED::PortValue{ED::inport_outflow_request, 5.0}},
    {ED::PortValue{ED::inport_inflow_achieved, 0.0}},
  };
  auto outputs = ED::run_devs<ED::StorageState>(
      [&](const ED::StorageState &s) { return ED::storage_time_advance(data, s); },
      [&](const ED::StorageState &s) { return ED::storage_internal_transition(data, s); },
      [&](const ED::StorageState &s, ED::RealTimeType e, const std::vector<ED::PortValue> &xs) {
        return ED::storage_external_transition(data, s, e, xs);
      },
      [&](const ED::StorageState &s, const std::vector<ED::PortValue> &xs) {
        return ED::storage_confluent_transition(data, s, xs);
      },
      [&](const ED::StorageState &s) { return ED::storage_output_function(data, s); },
      s0, times_s, xss, 100);
  if (outputs.size() != expected_times_s.size()) {
    std::cout << "Expected outputs.size() == expected_times_s.size()\n"
              << "outputs.size() = " << outputs.size() << "\n"
              << "expected_times_s.size() = "
              << expected_times_s.size() << "\n";
    return false;
  }
  for (size_type idx{0}; idx < outputs.size(); idx++) {
    const auto& out = outputs[idx];
    const auto& t_expected = expected_times_s[idx];
    const auto& soc_expected = expected_socs[idx];
    const auto& ia_expected = expected_inflows_achieved[idx];
    const auto& ir_expected = expected_inflows_requested[idx];
    const auto& oa_expected = expected_outflows_achieved[idx];
    const auto& or_expected = expected_outflows_requested[idx];
    if (out.time_s != t_expected) {
      std::cout << "event times do not meet expectation\n"
                << "idx        : " << idx << "\n"
                << "out.time_s : " << out.time_s << "\n"
                << "t_expected : " << t_expected << "\n";
      return false;
    }
    if (std::abs(out.state.soc - soc_expected) > tol) {
      std::cout << "event SOCs do not meet expectation\n"
                << "idx        : " << idx << "\n"
                << "out.time_s : " << out.state.soc << "\n"
                << "t_expected : " << soc_expected << "\n";
      return false;
    }
    if (std::abs(out.state.inflow_port.get_achieved() - ia_expected) > tol) {
      std::cout << "inflow achieved does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.inflow_port.get_achieved(): "
                << out.state.inflow_port.get_achieved() << "\n"
                << "ia_expected: "
                << ia_expected << "\n";
      return false;
    }
    if (std::abs(out.state.inflow_port.get_requested() - ir_expected) > tol) {
      std::cout << "inflow request does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.inflow_port.get_requested(): "
                << out.state.inflow_port.get_requested() << "\n"
                << "ir_expected: "
                << ir_expected << "\n";
      return false;
    }
    if (std::abs(out.state.outflow_port.get_achieved() - oa_expected) > tol) {
      std::cout << "outflow achieved does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.outflow_port.get_achieved(): "
                << out.state.outflow_port.get_achieved() << "\n"
                << "oa_expected: " << oa_expected << "\n";
      return false;
    }
    if (std::abs(out.state.outflow_port.get_requested() - or_expected) > tol) {
      std::cout << "outflow requested does not meet expectation\n"
                << "idx: " << idx << "\n"
                << "out.state.outflow_port.get_requested(): "
                << out.state.outflow_port.get_requested() << "\n"
                << "or_expected: " << or_expected << "\n";
      return false;
    }
    if (show_details) {
      ED::write_details<ED::StorageState>(out);
    }
  }
  return true;
}

int
main() {
  const bool show_details{true};
  int num_tests{0};
  int num_passed{0};
  bool current_test{true};
  std::cout << "test_undisturbed_discharge:\n";
  current_test = test_undisturbed_discharge(show_details);
  num_tests++;
  if (current_test) {
    num_passed++;
  }
  std::cout << "=> " << (current_test ? "PASSED" : "FAILED!!!!!") << "\n";
  ////
  std::cout << "Summary: " << num_passed << " / " << num_tests << " passed.\n";
  return 0;
}
