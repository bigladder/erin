#!/user/bin/env ruby
# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE file for additional terms and conditions.
######################################################################
# This is a unit test file for template.toml and various examples of exercising it.
# This module uses Ruby's standard unit test library, MiniTest
# This library assumes that Modelkit is installed and available from the
# commandline
require 'csv'
require 'fileutils'
require 'minitest/autorun'
require 'open3'
require 'set'
require 'stringio'
require_relative "support.rb"

THIS_DIR = File.expand_path(File.dirname(__FILE__))
REMOVE_FILES = true

class TestTemplate < Minitest::Test
  def setup
  end

  def teardown
  end

  def test_data_includes_connection
    data = {}
    Support.generate_connections_v2(data)
    assert(data.include?(:connection))
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
    Support.generate_connections(data)
    achieved = Set.new(data[:connection])
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
    Support.generate_connections(data)
    achieved = Set.new(data[:connection])
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
    Support.generate_connections(data)
    assert(data.include?(:muxer_component))
    assert(data.fetch(:muxer_component, []).length == 1)
    actual_mux_data = data.fetch(:muxer_component, []).fetch(0, {})
    expected_mux_data_keys = Set.new([:location_id, :id, :flow, :num_inflows, :num_outflows])
    actual_mux_data_keys = Set.new(actual_mux_data.keys)
    assert_equal(expected_mux_data_keys, actual_mux_data_keys)
    assert_equal(actual_mux_data.fetch(:location_id, ""), "b1")
    assert_equal(actual_mux_data.fetch(:id, ""), "b1_electricity_bus")
    assert_equal(actual_mux_data.fetch(:flow, ""), "electricity")
    assert_equal(actual_mux_data.fetch(:num_inflows, 0), 2)
    assert_equal(actual_mux_data.fetch(:num_outflows, 0), 1)
    achieved = Set.new(data[:connection])
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
    Support.generate_connections(data)
    achieved = Set.new(data[:connection])
    expected = Set.new([
      ["utility_electricity_source:OUT(0)", "b1_electricity_store:IN(0)", "electricity"],
      ["b1_electricity_store:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(data.fetch(:converter_component, []).length, 0)
    assert_equal(data.fetch(:storage_component, []).length, 1)
    assert_equal(data.fetch(:load_component, []).length, 1)
    assert_equal(data.fetch(:source_component, []).length, 1)
    assert_equal(data.fetch(:muxer_component, []).length, 0)
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
    Support.generate_connections(data)
    assert_equal(data.fetch(:load_component, []).length, 1)
    assert_equal(data.fetch(:source_component, []).length, 2)
    assert_equal(data.fetch(:converter_component, []).length, 1)
    assert_equal(data.fetch(:storage_component, []).length, 1)
    assert_equal(data.fetch(:muxer_component, []).length, 1)
    achieved = Set.new(data[:connection])
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
    Support.generate_connections(data)
    assert_equal(data.fetch(:load_component, []).length, 1)
    assert_equal(data.fetch(:source_component, []).length, 1)
    assert_equal(data.fetch(:converter_component, []).length, 1)
    assert_equal(data.fetch(:storage_component, []).length, 0)
    assert_equal(data.fetch(:muxer_component, []).length, 0)
    achieved = Set.new(data[:connection])
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
    Support.generate_connections(data)
    assert_equal(data.fetch(:load_component, []).length, 1)
    assert_equal(data.fetch(:source_component, []).length, 1)
    assert_equal(data.fetch(:converter_component, []).length, 0)
    assert_equal(data.fetch(:storage_component, []).length, 2)
    assert_equal(data.fetch(:muxer_component, []).length, 2)
    achieved = Set.new(data[:connection])
    expected = Set.new([
      ["utility_electricity_source:OUT(0)", "b1_electricity_bus:IN(0)", "electricity"],
      ["b1_electricity_bus:OUT(0)", "b1_electricity_store:IN(0)", "electricity"],
      ["b1_electricity_store:OUT(0)", "b1_electricity_bus_1:IN(0)", "electricity"],
      ["b1_electricity_bus:OUT(1)", "b1_electricity_store_1:IN(0)", "electricity"],
      ["b1_electricity_store_1:OUT(0)", "b1_electricity_bus_1:IN(1)", "electricity"],
      ["b1_electricity_bus_1:OUT(0)", "b1_electricity:IN(0)", "electricity"],
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
    Support.generate_connections(data)
    assert_equal(data.fetch(:load_component, []).length, 1)
    assert_equal(data.fetch(:source_component, []).length, 2)
    assert_equal(data.fetch(:converter_component, []).length, 0)
    assert_equal(data.fetch(:storage_component, []).length, 0)
    assert_equal(data.fetch(:muxer_component, []).length, 1)
    achieved = Set.new(data[:connection])
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
    Support.generate_connections_v2(data)
    assert_equal(data.fetch(:load_component, []).length, 1)
    assert_equal(data.fetch(:source_component, []).length, 1)
    assert_equal(data.fetch(:converter_component, []).length, 1)
    assert_equal(data.fetch(:storage_component, []).length, 0)
    assert_equal(data.fetch(:muxer_component, []).length, 0)
    achieved = Set.new(data[:connection])
    expected = Set.new([
      ["utility_natural_gas_source:OUT(0)", "cluster_electricity_generator:IN(0)", "natural_gas"],
      ["cluster_electricity_generator:OUT(0)", "b1_electricity:IN(0)", "electricity"],
    ])
    assert_equal(expected, achieved, "non-matches: #{achieved - expected}")
  end
end
