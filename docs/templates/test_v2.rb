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
end
