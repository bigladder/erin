#!/user/bin/env ruby
# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE file for additional terms and conditions.
######################################################################
# This is a unit test library for the support library for use in conjunction
# with template.toml. This module uses Ruby's standard unit test library,
# MiniTest This library assumes that Modelkit is installed and available from
# the commandline.
require 'csv'
require 'fileutils'
require 'minitest/autorun'
require 'open3'
require 'set'
require 'stringio'
require_relative "support.rb"

THIS_DIR = File.expand_path(File.dirname(__FILE__))

class TestTemplate < Minitest::Test

  def test_data_includes_connection
    data = {}
    s = Support.new(data)
    assert(s.connections.empty?)
  end

  def test_expanding_paths
    data = {
      load_profile: [
        {
          scenario_id: "s1",
          building_id: "b1",
          enduse: "electricity",
          file: "a.csv",
        },
      ],
    }
    s = Support.new(data, THIS_DIR)
    assert(s.load_profile.length == 1)
    assert_equal(File.basename(s.load_profile[0][:file]), "a.csv")
    assert(s.load_profile[0][:file].length > "a.csv".length)
  end

  def test_scenarios_having_intensities
    data = {
      scenario: [
        {
          id: "s1",
          duration_in_hours: 24*14,
          occurrence_distribution: "every_30_years",
          calc_reliability: "FALSE",
          max_occurrence: 1,
        },
        {
          id: "s2",
          duration_in_hours: 24*14,
          occurrence_distribution: "every_30_years",
          calc_reliability: "FALSE",
          max_occurrence: 1,
        },
      ],
      damage_intensity: [
        {
          scenario_id: "s2",
          name: "heat_F",
          value: 130.0,
        },
        {
          scenario_id: "s1",
          name: "wind_speed_mph",
          value: 150.0,
        },
        {
          scenario_id: "s1",
          name: "inundation_depth_ft",
          value: 12.0,
        },
      ],
    }
    s = Support.new(data)
    assert_equal(2, s.damage_intensities_for_scenario("s1").length)
  end

  def test_connecting_a_single_source_and_sink_electrically
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        }
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "electricity",
        },
      ],
    }
    s = Support.new(data)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_electricity_source:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved)
  end

  def test_connecting_3_sources_at_one_location_to_3_sinks_at_another
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
        {
          location_id: "b1",
          inflow: "heating",
        },
        {
          location_id: "b1",
          inflow: "dhw",
        },
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
        {
          location_id: "utility",
          outflow: "heating",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
        {
          location_id: "utility",
          outflow: "dhw",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "electricity",
        },
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "heating",
        },
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "dhw",
        },
      ],
    }
    s = Support.new(data)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_electricity_source:OUT(0)", "b1_electricity:IN(0)", "electricity"],
      ["utility_dhw_source:OUT(0)", "b1_dhw:IN(0)", "dhw"],
      ["utility_heating_source:OUT(0)", "b1_heating:IN(0)", "heating"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_connecting_utility_electricity_to_local_source_plus_egen
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
        {
          location_id: "utility",
          outflow: "natural_gas",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      converter_component: [
        {
          location_id: "b1",
          inflow: "natural_gas",
          outflow: "electricity",
          constant_efficiency: 0.5,
        },
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "electricity",
        },
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "natural_gas",
        },
      ],
    }
    s = Support.new(data)
    assert(s.muxer_component.length == 1)
    actual_mux_data = s.muxer_component.fetch(0, {})
    expected_mux_data_keys = Set.new([:location_id, :id, :flow, :num_inflows, :num_outflows])
    actual_mux_data_keys = Set.new(actual_mux_data.keys)
    assert_equal(expected_mux_data_keys, actual_mux_data_keys)
    assert_equal(actual_mux_data.fetch(:location_id, ""), "b1")
    assert_equal(actual_mux_data.fetch(:id, ""), "b1_electricity_bus")
    assert_equal(actual_mux_data.fetch(:flow, ""), "electricity")
    assert_equal(actual_mux_data.fetch(:num_inflows, 0), 2)
    assert_equal(actual_mux_data.fetch(:num_outflows, 0), 1)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_electricity_source:OUT(0)", "b1_electricity_bus:IN(0)", "electricity"],
      ["utility_natural_gas_source:OUT(0)", "b1_electricity_generator:IN(0)", "natural_gas"],
      ["b1_electricity_generator:OUT(0)", "b1_electricity_bus:IN(1)", "electricity"],
      ["b1_electricity_bus:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_connecting_utility_electricity_to_local_source_plus_battery
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      storage_component: [
        {
          location_id: "b1",
          flow: "electricity",
          capacity_kWh: 100.0,
          max_inflow_kW: 10.0,
        },
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "electricity",
        },
      ],
    }
    s = Support.new(data)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_electricity_source:OUT(0)", "b1_electricity_store:IN(0)", "electricity"],
      ["b1_electricity_store:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(s.converter_component.length, 0)
    assert_equal(s.storage_component.length, 1)
    assert_equal(s.load_component.length, 1)
    assert_equal(s.source_component.length, 1)
    assert_equal(s.muxer_component.length, 0)
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_connecting_utility_electricity_to_local_source_plus_battery_and_egen
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
        {
          location_id: "utility",
          outflow: "natural_gas",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      converter_component: [
        {
          location_id: "b1",
          inflow: "natural_gas",
          outflow: "electricity",
          constant_efficiency: 0.5,
        },
      ],
      storage_component: [
        {
          location_id: "b1",
          flow: "electricity",
          capacity_kWh: 100.0,
          max_inflow_kW: 10.0,
        },
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "electricity",
        },
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "natural_gas",
        },
      ],
    }
    s = Support.new(data)
    assert_equal(s.load_component.length, 1)
    assert_equal(s.source_component.length, 2)
    assert_equal(s.converter_component.length, 1)
    assert_equal(s.storage_component.length, 1)
    assert_equal(s.muxer_component.length, 1)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_natural_gas_source:OUT(0)", "b1_electricity_generator:IN(0)", "natural_gas"],
      ["b1_electricity_generator:OUT(0)", "b1_electricity_bus:IN(1)", "electricity"],
      ["utility_electricity_source:OUT(0)", "b1_electricity_bus:IN(0)", "electricity"],
      ["b1_electricity_bus:OUT(0)", "b1_electricity_store:IN(0)", "electricity"],
      ["b1_electricity_store:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_local_egen_only_for_electric_load
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "natural_gas",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      converter_component: [
        {
          location_id: "b1",
          inflow: "natural_gas",
          outflow: "electricity",
          constant_efficiency: 0.5,
        },
      ],
      storage_component: [
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "natural_gas",
        },
      ],
    }
    s = Support.new(data)
    assert_equal(s.load_component.length, 1)
    assert_equal(s.source_component.length, 1)
    assert_equal(s.converter_component.length, 1)
    assert_equal(s.storage_component.length, 0)
    assert_equal(s.muxer_component.length, 0)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_natural_gas_source:OUT(0)", "b1_electricity_generator:IN(0)", "natural_gas"],
      ["b1_electricity_generator:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_multiple_parallel_stores_for_remote_supplied_electric_load
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      converter_component: [
      ],
      storage_component: [
        {
          location_id: "b1",
          flow: "electricity",
          capacity_kWh: 100.0,
          max_inflow_kW: 10.0,
        },
        {
          location_id: "b1",
          flow: "electricity",
          capacity_kWh: 50.0,
          max_inflow_kW: 5.0,
        },
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "electricity",
        },
      ],
    }
    s = Support.new(data)
    assert_equal(s.load_component.length, 1)
    assert_equal(s.source_component.length, 1)
    assert_equal(s.converter_component.length, 0)
    assert_equal(s.storage_component.length, 2)
    assert_equal(s.muxer_component.length, 2)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_electricity_source:OUT(0)", "b1_electricity_bus_1:IN(0)", "electricity"],
      ["b1_electricity_bus_1:OUT(0)", "b1_electricity_store:IN(0)", "electricity"],
      ["b1_electricity_store:OUT(0)", "b1_electricity_bus:IN(0)", "electricity"],
      ["b1_electricity_bus_1:OUT(1)", "b1_electricity_store_1:IN(0)", "electricity"],
      ["b1_electricity_store_1:OUT(0)", "b1_electricity_bus:IN(1)", "electricity"],
      ["b1_electricity_bus:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_multiple_sources_for_a_load
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      source_component: [
        {
          location_id: "c1",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
        {
          location_id: "c2",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      converter_component: [
      ],
      storage_component: [
      ],
      network_link: [
        {
          source_location_id: "c1",
          destination_location_id: "b1",
          flow: "electricity",
        },
        {
          source_location_id: "c2",
          destination_location_id: "b1",
          flow: "electricity",
        },
      ],
    }
    s = Support.new(data)
    assert_equal(s.load_component.length, 1)
    assert_equal(s.source_component.length, 2)
    assert_equal(s.converter_component.length, 0)
    assert_equal(s.storage_component.length, 0)
    assert_equal(s.muxer_component.length, 1)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["c1_electricity_source:OUT(0)", "b1_electricity_bus:IN(0)", "electricity"],
      ["c2_electricity_source:OUT(0)", "b1_electricity_bus:IN(1)", "electricity"],
      ["b1_electricity_bus:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_cluster_level_electrical_generation_from_utility_natural_gas
    data = {
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "natural_gas",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      converter_component: [
        {
          location_id: "cluster",
          inflow: "natural_gas",
          outflow: "electricity",
          constant_efficiency: 0.5,
        },
      ],
      storage_component: [
      ],
      network_link: [
        {
          source_location_id: "cluster",
          destination_location_id: "b1",
          flow: "electricity",
        },
        {
          source_location_id: "utility",
          destination_location_id: "cluster",
          flow: "natural_gas",
        },
      ],
    }
    s = Support.new(data)
    assert_equal(s.load_component.length, 1)
    assert_equal(s.source_component.length, 1)
    assert_equal(s.converter_component.length, 1)
    assert_equal(s.storage_component.length, 0)
    assert_equal(s.muxer_component.length, 0)
    achieved = Set.new(s.connections)
    expected = Set.new([
      ["utility_natural_gas_source:OUT(0)", "cluster_electricity_generator:IN(0)", "natural_gas"],
      ["cluster_electricity_generator:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end

  def test_getting_fragility_curve_data
    data = {}
    s = Support.new(data)
    assert_equal(0, s.fragility_curve.length)
    data = {
      fragility_curve: [
        {
          id: "highly_vulnerable_to_inundation",
          vulnerable_to: "inundation_height_ft",
          lower_bound: 6.0,
          upper_bound: 12.0,
        },
        {
          id: "somewhat_vulnerable_to_wind",
          vulnerable_to: "wind_speed_mph",
          lower_bound: 120.0,
          upper_bound: 200.0,
        },
        {
          id: "does_it_blend?",
          vulnerable_to: "blending",
          lower_bound: 2.0,
          upper_bound: 10.0,
        },
      ],
      component_fragility: [
        {
          component_id: "b1_boiler",
          fragility_id: "highly_vulnerable_to_inundation",
        },
        {
          component_id: "b1_boiler",
          fragility_id: "somewhat_vulnerable_to_wind",
        },
        {
          component_id: "b2_boiler",
          fragility_id: "does_it_blend?",
        },
      ],
    }
    s = Support.new(data)
    assert_equal(3, s.fragility_curve.length)
    assert_equal(
      Set.new(["somewhat_vulnerable_to_wind", "highly_vulnerable_to_inundation"]),
      Set.new(s.fragilities_for_component("b1_boiler").map {|f|f[:id]}))
  end

  def test_getting_failure_mode_data
    data = {}
    s = Support.new(data)
    assert_equal(0, s.failure_mode.length)
    data = {
      component_failure_mode: [
        component_id: "utility_electricity_source",
        failure_mode_id: "typical_utility_reliability",
      ],
      failure_mode: [
        {
          id: "typical_utility_reliability",
          failure_cdf: "fixed_10_years",
          repair_cdf: "fixed_4_hours",
        },
        {
          id: "boiler_reliability",
          failure_cdf: "fixed_10_years",
          repair_cdf: "uniform_range_4_to_36_hours",
        },
        {
          id: "electric_generator_starter",
          failure_cdf: "fixed_10_years",
          repair_cdf: "uniform_range_4_to_36_hours",
        },
      ],
      fixed_cdf: [
        {
          id: "fixed_10_years",
          value_in_hours: 8760 * 10,
        },
        {
          id: "fixed_4_hours",
          value_in_hours: 4,
        },
      ],
    }
    s = Support.new(data)
    assert_equal(3, s.failure_mode.length)
    assert_equal(
      Set.new(["typical_utility_reliability"]),
      Set.new(s.failure_modes_for_component("utility_electricity_source").map {|f|f[:id]}))
  end

  def test_pass_through_components_on_network_links
    data = {}
    s = Support.new(data)
    assert_equal(0, s.pass_through_component.length)
    data = {
      source_component: [
        {
          location_id: "c1",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      network_link: [
        {
          source_location_id: "c1",
          destination_location_id: "b1",
          flow: "electricity",
        },
      ],
      pass_through_component: [
        {
          id: "electric_lines",
          link_id: "c1_to_b1_electricity",
          flow: "electricity",
        },
      ]
    }
    s = Support.new(data)
    assert_equal(1, s.pass_through_component.length)
    expected_conns = Set.new([
      ["c1_electricity_source:OUT(0)", "electric_lines:IN(0)", "electricity"],
      ["electric_lines:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    actual_conns = Set.new(s.connections)
    assert_equal(expected_conns, actual_conns)
    data = {
      source_component: [
        {
          location_id: "c1",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
      ],
      network_link: [
        {
          source_location_id: "c1",
          destination_location_id: "b1",
          flow: "electricity",
        },
      ],
      pass_through_component: [
        {
          id: "electric_lines_A",
          link_id: "c1_to_b1_electricity",
          flow: "electricity",
        },
        {
          id: "electric_lines_B",
          link_id: "c1_to_b1_electricity",
          flow: "electricity",
        },
      ]
    }
    s = Support.new(data)
    assert_equal(2, s.pass_through_component.length)
    assert_equal(2, s.muxer_component.length)
    expected_conns = Set.new([
      ["c1_electricity_source:OUT(0)", "c1_to_b1_electricity_inflow_bus:IN(0)", "electricity"],
      ["c1_to_b1_electricity_inflow_bus:OUT(0)", "electric_lines_A:IN(0)", "electricity"],
      ["electric_lines_A:OUT(0)", "c1_to_b1_electricity_outflow_bus:IN(0)", "electricity"],
      ["c1_to_b1_electricity_inflow_bus:OUT(1)", "electric_lines_B:IN(0)", "electricity"],
      ["electric_lines_B:OUT(0)", "c1_to_b1_electricity_outflow_bus:IN(1)", "electricity"],
      ["c1_to_b1_electricity_outflow_bus:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    actual_conns = Set.new(s.connections)
    assert_equal(expected_conns, actual_conns)
  end

  def test_data_read
    data = {
      component_failure_mode: [
        {
          component_id: "utility_electricity_source",
          failure_mode_id: "typical_utility_failures"
        }
      ],
      component_fragility: [],
      converter_component: [],
      damage_intensity: [],
      failure_mode: [
        {
          id: "typical_utility_failures",
          failure_cdf: "every_10_hours",
          repair_cdf: "every_4_years"
        }
      ],
      fixed_cdf: [
        {
          id: "every_10_hours",
          value_in_hours: "10"
        },
        {
          id: "every_4_years",
          value_in_hours: "35040"
        }
      ],
      fragility_curve: [],
      general: {
        simulation_duration_in_years: "100",
        random_setting: "Auto",
        random_seed: "17"
      },
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity"
        }
      ],
      load_profile: [
        {
          scenario_id: "s1",
          building_id: "b1",
          enduse: "electricity",
          file: "s1_b1_electricity.csv"
        }
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "electricity"
        }
      ],
      pass_through_component: [],
      scenario: [
        {
          id: "s1",
          duration_in_hours: "336",
          occurrence_distribution: "every_30_years",
          calc_reliability: "false",
          max_occurrence: "-1"
        }
      ],
      source_component: [
        {
          location_id: "utility",
          outflow: "electricity",
          is_limited: "FALSE",
          max_outflow_kW: "0.0"
        }
      ],
      storage_component: []
    }
    _ = Support.new(data)
  end

  def test_add_dual_output_converter_component
    data = {
      source_component: [
        {
          location_id: "utility",
          outflow: "natural_gas",
          is_limited: "FALSE",
          max_outflow_kW: 0.0,
        },
      ],
      load_component: [
        {
          location_id: "b1",
          inflow: "electricity",
        },
        {
          location_id: "b1",
          inflow: "heating",
        },
      ],
      dual_outflow_converter_component: [
        {
          location_id: "b1",
          inflow: "natural_gas",
          primary_outflow: "electricity",
          secondary_outflow: "heating",
          lossflow: "waste_heat",
          primary_efficiency: 0.4,
          secondary_efficiency: 0.8,
        },
      ],
      network_link: [
        {
          source_location_id: "utility",
          destination_location_id: "b1",
          flow: "natural_gas",
        },
      ],
    }
    s = Support.new(data)
    assert_equal(2, s.converter_component.length)
    #data = {
    #  source_component: [
    #    {
    #      location_id: "c1",
    #      outflow: "electricity",
    #      is_limited: "FALSE",
    #      max_outflow_kW: 0.0,
    #    },
    #  ],
    #  load_component: [
    #    {
    #      location_id: "b1",
    #      inflow: "electricity",
    #    },
    #  ],
    #  network_link: [
    #    {
    #      source_location_id: "c1",
    #      destination_location_id: "b1",
    #      flow: "electricity",
    #    },
    #  ],
    #  pass_through_component: [
    #    {
    #      id: "electric_lines",
    #      link_id: "c1_to_b1_electricity",
    #      flow: "electricity",
    #    },
    #  ]
    #}
    #s = Support.new(data)
    #assert_equal(1, s.pass_through_component.length)
    #expected_conns = Set.new([
    #  ["c1_electricity_source:OUT(0)", "electric_lines:IN(0)", "electricity"],
    #  ["electric_lines:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    #])
    #actual_conns = Set.new(s.connections)
    #assert_equal(expected_conns, actual_conns)
  end
end
