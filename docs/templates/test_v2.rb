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
    Support.generate_connections(data)
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
    puts "="*60 + "\n"
    Support.generate_connections(data)
    puts "="*60 + "\n"
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
end
