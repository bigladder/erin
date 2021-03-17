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
test_for_bad_energy_balance(bool show_details)
{
  auto data = ED::storage_make_data(100.0, 10.0);
  auto s0 = ED::storage_make_state(data, 1.0);
  const std::vector<ED::RealTimeType> times_s{
    0,
    0,
    20,
    25,
    30,
    40,
  };
  const std::vector<std::vector<ED::PortValue>> xss{
    {ED::PortValue{ED::inport_outflow_request, 5.0}},
    {ED::PortValue{ED::inport_inflow_achieved, 0.0}},
    {ED::PortValue{ED::inport_outflow_request, 50.0}},
    {ED::PortValue{ED::inport_outflow_request, 0.0}},
    {ED::PortValue{ED::inport_inflow_achieved, 40.0}},
    {ED::PortValue{ED::inport_outflow_request, 100.0}},
  };
  auto outputs = ED::run_devs_v2<ED::StorageState>(
      [&](const ED::StorageState &s) { return ED::storage_time_advance(data, s); },
      [&](const ED::StorageState &s) { return ED::storage_internal_transition(data, s); },
      [&](const ED::StorageState &s, ED::RealTimeType e, const std::vector<ED::PortValue> &xs) {
        return ED::storage_external_transition(data, s, e, xs);
      },
      [&](const ED::StorageState &s, const std::vector<ED::PortValue> &xs) {
        return ED::storage_confluent_transition(data, s, xs);
      },
      [&](const ED::StorageState &s) { return ED::storage_output_function(data, s); },
      s0,
      times_s,
      xss,
      100,
      [&](const ED::StorageState &s, const ED::EnergyAudit& ea, const ED::RealTimeType& dt) {
        if (dt == 0) {
          return ea;
        }
        return ED::EnergyAudit{
          ea.in + s.inflow_port.get_achieved() * dt,
          ea.out + s.outflow_port.get_achieved() * dt,
          ea.waste,
          ea.store + (s.inflow_port.get_achieved() - s.outflow_port.get_achieved()) * dt};
      });
  for (const auto& item : outputs) {
    if (show_details) {
      ED::write_details_v2(item);
    }
    auto error = ED::energy_audit_error(item.energy);
    if (std::abs(error) > 1e-6) {
      return false;
    }
  }
  return true;
}

int
main() {
  const bool show_details{false};
  int num_tests{0};
  int num_passed{0};
  bool current_test{true};
  std::cout << "test_for_bad_energy_balance:\n";
  current_test = test_for_bad_energy_balance(show_details);
  num_tests++;
  if (current_test) {
    num_passed++;
  }
  std::cout << "=> " << (current_test ? "PASSED" : "FAILED!!!!!") << "\n";
  std::cout << "Summary: " << num_passed << " / " << num_tests << " passed.\n";
  if (num_passed == num_tests) {
    return 0;
  }
  return 1;
}
