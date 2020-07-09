#!/user/bin/env ruby
# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE file for additional terms and conditions.
######################################################################
# This is a unit test file for template.toml and various examples of exercising it.
# This module uses Ruby's standard unit test library, MiniTest
# This library assumes that Modelkit is installed and available from the
# commandline
require 'fileutils'
require 'minitest/autorun'
require 'open3'
require 'set'
require 'stringio'
require_relative "support.rb"

THIS_DIR = File.expand_path(File.dirname(__FILE__))
REMOVE_FILES = true

class TestTemplate < Minitest::Test
  # RETURN: (Hash symbol any), the default parameters for the template
  def default_params
    {
      # General
      :simulation_duration_in_years => 100,
      :random_setting => "Auto",
      :random_seed => 17,
      # Load Profile
      :load_profile_scenario_id => ["blue_sky", "blue_sky"],
      :load_profile_building_id => ["mc", "other"],
      :load_profile_enduse => ["electricity", "electricity"],
      :load_profile_file => ["mc_blue_sky_electricity.csv", "other_blue_sky_electricity.csv"],
      # Scenario
      :scenario_id => ["blue_sky"],
      :scenario_duration_in_hours => [8760],
      :scenario_max_occurrence => [1],
      :scenario_fixed_frequency_in_years => [0],
      # Building Level Configuration
      :building_level_building_id => ["mc", "other"],
      :building_level_egen_flag => ["FALSE", "FALSE"],
      :building_level_egen_eff_pct => [32.0, 32.0],
      #:building_level_egen_peak_pwr_kW => [100.0, 100.0],
      :building_level_heat_storage_flag => ["FALSE", "FALSE"],
      :building_level_heat_storage_cap_kWh => [0.0, 0.0],
      :building_level_gas_boiler_flag => ["FALSE", "FALSE"],
      :building_level_gas_boiler_eff_pct => [85.0, 85.0],
      :building_level_electricity_supply_node => ["utility", "utility"],
      #:building_level_gas_boiler_peak_heat_gen_kW => [50.0, 50.0],
      #:building_level_echiller_flag => ["FALSE", "FALSE"],
      #:building_level_echiller_peak_cooling_kW => [50.0, 50.0],
      # Node Level Configuration -- Cluster and Community
      :node_level_id => [],
      :node_level_ng_power_plant_flag => [],
      :node_level_ng_power_plant_eff_pct => [],
      :node_level_ng_supply_node => [],
    }
  end

  # RETURN: (Hash symbol any), the default parameters for the template
  def multiple_scenarios_params
    ps = default_params
    ps[:load_profile_scenario_id] += ["s1"]*2 + ["s2"]*2
    ps[:load_profile_building_id] += ["mc", "other"]*2
    ps[:load_profile_enduse] += ["electricity"]*4
    ps[:load_profile_file] += [
      "mc_s1_electricity.csv",
      "other_s1_electricity.csv",
      "mc_s2_electricity.csv",
      "other_s2_electricity.csv",
    ]
    ps[:scenario_id] += ["s1", "s2"]
    ps[:scenario_duration_in_hours] += [500]*2
    ps[:scenario_max_occurrence] += [-1]*2
    ps[:scenario_fixed_frequency_in_years] += [10]*2
    ps
  end

  def add_one_electric_generator_at_building_level_params
    ps = default_params
    ps[:building_level_egen_flag][0] = "TRUE"
    ps[:building_level_egen_eff_pct][0] = 42.0
    ps
  end

  def only_one_building_with_electric_loads_params
    ps = default_params
    ps[:load_profile_scenario_id] = ["blue_sky"]
    ps[:load_profile_building_id] = ["b1"]
    ps[:load_profile_enduse] = ["electricity"]
    ps[:load_profile_file] = ["b1_blue_sky_electricity.csv"]
    ps[:building_level_building_id] = ["b1"]
    ps[:building_level_egen_flag] = ["FALSE"]
    ps[:building_level_egen_eff_pct] = [32.0]
    ps[:building_level_heat_storage_flag] = ["FALSE"]
    ps[:building_level_heat_storage_cap_kWh] = [0.0]
    ps[:building_level_gas_boiler_flag] = ["FALSE"]
    ps[:building_level_gas_boiler_eff_pct] = [85.0]
    ps[:building_level_electricity_supply_node] = ["utility"]
    ps
  end

  def one_building_has_thermal_storage_params
    ps = default_params
    ps[:load_profile_scenario_id] << "blue_sky"
    ps[:load_profile_building_id] << "mc"
    ps[:load_profile_enduse] << "heating"
    ps[:load_profile_file] << "mc_blue_sky_heating.csv"
    ps[:building_level_heat_storage_flag][0] = "TRUE"
    ps[:building_level_heat_storage_cap_kWh][0] = 100.0
    ps
  end

  # RETURN: string, path to the e2rin_multi executable
  def e2rin_path
    path1 = File.join(THIS_DIR, '..', '..', 'build', 'bin', 'e2rin_multi')
    return path1 if File.exist?(path1)
  end

  def e2rin_graph_path
    path1 = File.join(THIS_DIR, '..', '..', 'build', 'bin', 'e2rin_graph')
    return path1 if File.exist?(path1)
  end

  # - input_tag: string, the tag for the input file, such as 'defaults', for reference/defaults.toml
  # - load_profiles: (array string), the csv files for the load profiles
  # RETURN: bool
  def run_e2rin(input_tag, load_profiles)
    load_profiles.each do |path|
      File.open(path, 'w') do |f|
        f.write("hours,kW\n")
        f.write("0,1\n")
        f.write("10000,0\n")
      end
    end
    input_path = File.join(THIS_DIR, 'reference', input_tag + ".toml")
    `#{e2rin_path} #{input_path} out.csv stats.csv`
    success = ($?.to_i == 0)
    (load_profiles + ['out.csv', 'stats.csv']).each do |path|
      File.delete(path) if File.exist?(path)
    end
    return success
  end

  # - input_tag: string, the tag for the input file, such as 'defaults', for reference/defaults.toml
  # - load_profiles: (array string), the csv files for the load profiles
  # RETURN: bool
  def run_e2rin_graph(input_tag, load_profiles)
    load_profiles.each do |path|
      File.open(path, 'w') do |f|
        f.write("hours,kW\n")
        f.write("0,1\n")
        f.write("10000,0\n")
      end
    end
    input_path = File.join(THIS_DIR, 'reference', input_tag + ".toml")
    `#{e2rin_graph_path} #{input_path} #{input_tag}.gv nw`
    success = ($?.to_i == 0)
    `dot -Tpng #{input_tag}.gv -o #{input_tag}.png`
    success = success and ($?.to_i == 0)
    (load_profiles + ['out.csv', 'stats.csv']).each do |path|
      File.delete(path) if File.exist?(path)
    end
    return success
  end

  # - params: (Hash symbol any), the parameters to write
  # - path: string, path to the pxt file to write out
  # RETURN: nil
  def write_params(params, path)
    File.write(path, params.to_s.gsub(/^{/, '').gsub(/}$/, '').gsub(/, :/, ",\n:"))
  end

  # - args: string, arguments to be appended after "modelkit template-compose "
  # - output_file: string, path to the output file to expect
  # RETURN: {
  #   cmd: string, the command run,
  #   success: bool, true if successful, else false
  #   exit: bool, true if we got a 0 exit code form modelkit
  #   output_exists: bool, true if output was rendered
  #   output: (Or nil string), if output from rendered template exists, place here; else nil
  #   stdout: string, standard output
  #   stderr: string, standard error
  #   }
  def call_modelkit(pxt_file, tmp_file, output_file)
    args = "--output=\"#{output_file}\" --files=\"#{pxt_file}\" \"#{tmp_file}\""
    cmd = "modelkit template-compose #{args}"
    val = {}
    out = ""
    err = ""
    `modelkit template-compose #{args}`
    success = ($?.to_i == 0)
    exist = File.exist?(output_file)
    val[:cmd] = cmd
    val[:success] = success && exist
    val[:exit] = success
    val[:output_exists] = exist
    val[:output] = nil
    val[:stdout] = out
    val[:stderr] = err
    val[:output] = File.read(output_file) if exist
    val[:pxt] = if File.exist?(pxt_file) then File.read(pxt_file) else nil end
    return val
  end

  # - v: (Hash ...), any hash
  # RETURN: string, a print-out of the hash into a string
  def error_string(v)
    s = StringIO.new
    s.write("ERROR:\n")
    v.keys.sort.each do |key|
      s.write("#{key.to_s}: #{v[key]}\n")
    end
    s.string
  end

  # RETURN: nil
  # SIDE-EFFECTS:
  # - ensures the @output_file and @params_file are removed if exist
  def ensure_directory_clean
    [@output_file, @params_file].each do |path|
      File.delete(path) if File.exist?(path) and REMOVE_FILES
    end
  end

  def setup
    @params_file = "test.pxt"
    @template_file = "template.toml"
    @output_file = "test.toml"
    ensure_directory_clean
  end

  def teardown
    ensure_directory_clean
  end

  # - params: (Hash ..), the params
  # - reference_tag: string, tag identifying the reference file to compare to; e.g., 'default' for reference/default.toml
  # RETURN: nil
  # SIDE-EFFECTS:
  # - runs modelkit and asserts output is the same as reference
  def run_and_compare(params, reference_tag, save_rendered_template = true)
    write_params(params, @params_file)
    out = call_modelkit(@params_file, @template_file, @output_file)
    if File.exist?(@output_file) and save_rendered_template
      FileUtils.cp(@output_file, reference_tag + '.toml')
    end
    err_str = error_string(out)
    assert(out[:success], err_str)
    assert(!out[:output].nil?, err_str)
    expected = File.read(File.join('reference', "#{reference_tag}.toml"))
    assert_equal(out[:output], expected, err_str)
  end

  def test_defaults
    ps = default_params
    run_and_compare(ps, 'defaults')
    assert(run_e2rin('defaults', ps[:load_profile_file]))
    assert(run_e2rin_graph('defaults', ps[:load_profile_file]))
  end

  def test_multiple_scenarios
    ps = multiple_scenarios_params
    run_and_compare(ps, 'multiple_scenarios')
    assert(run_e2rin('multiple_scenarios', ps[:load_profile_file]))
    assert(run_e2rin_graph('multiple_scenarios', ps[:load_profile_file]))
  end

  def test_add_one_electric_generator_at_building_level
    ps = add_one_electric_generator_at_building_level_params
    run_and_compare(ps, 'add_one_electric_generator_at_building_level')
    assert(
      run_e2rin(
        'add_one_electric_generator_at_building_level',
        ps[:load_profile_file]))
    assert(
      run_e2rin_graph(
        'add_one_electric_generator_at_building_level',
        ps[:load_profile_file]))
  end

  def test_only_one_building_with_electric_loads
    ps = only_one_building_with_electric_loads_params
    run_and_compare(ps, 'only_one_building_with_electric_loads')
    assert(
      run_e2rin(
        'only_one_building_with_electric_loads', ps[:load_profile_file]))
    assert(
      run_e2rin_graph(
        'only_one_building_with_electric_loads',
        ps[:load_profile_file]))
  end

  def test_one_building_has_thermal_storage
    ps = one_building_has_thermal_storage_params
    run_and_compare(ps, 'one_building_has_thermal_storage')
    assert(
      run_e2rin(
        'one_building_has_thermal_storage', ps[:load_profile_file]))
    assert(
      run_e2rin_graph(
        'one_building_has_thermal_storage', ps[:load_profile_file]))
  end

  def test_one_building_has_boiler
    ps = default_params
    ps[:load_profile_scenario_id] << "blue_sky"
    ps[:load_profile_building_id] << "mc"
    ps[:load_profile_enduse] << "heating"
    ps[:load_profile_file] << "mc_blue_sky_heating.csv"
    ps[:building_level_gas_boiler_flag][0] = "TRUE"
    ps[:building_level_gas_boiler_eff_pct][0] = 85.0
    run_and_compare(ps, 'one_building_has_boiler')
    assert(
      run_e2rin(
        'one_building_has_boiler', ps[:load_profile_file]))
    assert(
      run_e2rin_graph(
        'one_building_has_boiler', ps[:load_profile_file]))
  end

  def test_all_building_config_options_true
    ps = default_params
    ps[:load_profile_scenario_id] += ["blue_sky"]*2
    ps[:load_profile_building_id] += ["mc", "other"]
    ps[:load_profile_enduse] += ["heating"]*2
    ps[:load_profile_file] += ["mc_blue_sky_heating.csv", "other_blue_sky_heating.csv"]
    ps[:building_level_egen_flag] = ["TRUE"]*2
    ps[:building_level_egen_eff_pct] = [32.0, 32.0]
    ps[:building_level_heat_storage_flag] = ["TRUE"]*2
    ps[:building_level_heat_storage_cap_kWh] = [100.0]*2
    ps[:building_level_gas_boiler_flag] = ["TRUE"]*2
    ps[:building_level_gas_boiler_eff_pct] = [85.0]*2
    run_and_compare(ps, 'all_building_config_options_true')
    assert(
      run_e2rin(
        'all_building_config_options_true', ps[:load_profile_file]))
    assert(
      run_e2rin_graph(
        'all_building_config_options_true', ps[:load_profile_file]))
  end

  #def test_electric_generation_at_the_cluster_level
  #  ps = {
  #    # General
  #    :simulation_duration_in_years => 100,
  #    :random_setting => "Auto",
  #    :random_seed => 17,
  #    # Load Profile
  #    :load_profile_scenario_id => ["blue_sky", "blue_sky"],
  #    :load_profile_building_id => ["mc", "other"],
  #    :load_profile_enduse => ["electricity", "electricity"],
  #    :load_profile_file => ["mc_blue_sky_electricity.csv", "other_blue_sky_electricity.csv"],
  #    # Scenario
  #    :scenario_id => ["blue_sky"],
  #    :scenario_duration_in_hours => [8760],
  #    :scenario_max_occurrence => [1],
  #    :scenario_fixed_frequency_in_years => [0],
  #    # Building Level Configuration
  #    :building_level_building_id => ["mc", "other"],
  #    :building_level_egen_flag => ["FALSE", "FALSE"],
  #    :building_level_egen_eff_pct => [32.0, 32.0],
  #    #:building_level_egen_peak_pwr_kW => [100.0, 100.0],
  #    :building_level_heat_storage_flag => ["FALSE", "FALSE"],
  #    :building_level_heat_storage_cap_kWh => [0.0, 0.0],
  #    :building_level_gas_boiler_flag => ["FALSE", "FALSE"],
  #    :building_level_gas_boiler_eff_pct => [85.0, 85.0],
  #    :building_level_electricity_supply_node => ["cluster_0", "cluster_0"],
  #    #:building_level_gas_boiler_peak_heat_gen_kW => [50.0, 50.0],
  #    #:building_level_echiller_flag => ["FALSE", "FALSE"],
  #    #:building_level_echiller_peak_cooling_kW => [50.0, 50.0],
  #    # Node Level Configuration
  #    :node_level_id => ["cluster_0"],
  #    :node_level_ng_power_plant_flag => ["TRUE"],
  #    :node_level_ng_power_plant_eff_pct => [42.0],
  #    #:node_level_ng_chp_flag => ["FALSE"],
  #    #:node_level_ng_boiler_flag => ["FALSE"],
  #    #:node_level_tes_flag => ["FALSE"],
  #    #:node_level_absorption_chiller_flag => ["FALSE", "FALSE"],
  #    #:node_level_electric_chiller_flag => ["FALSE", "FALSE"],
  #    :node_level_ng_supply_node => ["utility"],
  #    # Community Level Configuration
  #  }
  #  run_and_compare(ps, 'electric_generation_at_the_cluster_level', true)
  #  assert(
  #    run_e2rin(
  #      'electric_generation_at_the_cluster_level', ps[:load_profile_file]))
  #  assert(
  #    run_e2rin_graph(
  #      'electric_generation_at_the_cluster_level', ps[:load_profile_file]))
  #end

  def make_support_instance(ps)
    Support.new(
      ps[:load_profile_scenario_id],
      ps[:load_profile_building_id],
      ps[:load_profile_enduse],
      ps[:load_profile_file],
      ps[:building_level_building_id],
      ps[:building_level_egen_flag],
      ps[:building_level_egen_eff_pct],
      ps[:building_level_heat_storage_flag],
      ps[:building_level_heat_storage_cap_kWh],
      ps[:building_level_gas_boiler_flag],
      ps[:building_level_gas_boiler_eff_pct],
      ps[:building_level_electricity_supply_node],
      ps[:node_level_id],
      ps[:node_level_ng_power_plant_flag],
      ps[:node_level_ng_power_plant_eff_pct],
      ps[:node_level_ng_supply_node]
    )
  end

  def compare_support_lib_outputs(s, expected_comps, expected_conns, verbose = false)
    comps = s.components
    expected_comp_ids = Set.new(expected_comps.map {|c| c[:id]})
    actual_comp_ids = Set.new(comps.map {|c| c[:id]})
    assert_equal(expected_comp_ids, actual_comp_ids)
    expected_comp_values = Set.new(expected_comps.map {|c| c[:string]})
    actual_comp_values = Set.new(comps.map {|c| c[:string]})
    actual_comp_values.each do |item|
      msg = ""
      if verbose
        msg += "\nactual:\n#{item}\n"
        msg += "="*60 + "\n"
        expected_comp_values.each_with_index do |x, idx|
          msg += "\nexpected(#{idx}):\n#{x}\n"
          msg += "-"*60 + "\n"
        end
        msg += ("="*60 + "\n")
      end
      assert(expected_comp_values.include?(item), "#{item} not in expected set\n#{msg}\n")
    end
    assert_equal(expected_comp_values.size, actual_comp_values.size)
    assert_equal(expected_comp_values, actual_comp_values)
    conns = s.connections
    expected_conns = Set.new(expected_conns)
    actual_conns = Set.new(conns)
    assert_equal(expected_conns, actual_conns)
  end

  def test_support_lib_with_defaults
    compare_support_lib_outputs(
      make_support_instance(default_params),
      [
        {
          id: "mc_electricity",
          string: "[components.mc_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"mc_electricity_blue_sky\"\n"
        },
        {
          id: "other_electricity",
          string: "[components.other_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"other_electricity_blue_sky\"\n"
        },
        {
          id: "utility_electricity_bus",
          string: "[components.utility_electricity_bus]\n"\
          "type = \"muxer\"\n"\
          "stream = \"electricity\"\n"\
          "num_inflows = 1\n"\
          "num_outflows = 2\n"\
          "dispatch_strategy = \"in_order\"\n",
        },
        {
          id: "utility_electricity_source",
          string: "[components.utility_electricity_source]\n"\
          "type = \"source\"\n"\
          "outflow = \"electricity\"\n"
        }
      ],
      [ ["utility_electricity_source:OUT(0)", "utility_electricity_bus:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(0)", "mc_electricity:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(1)", "other_electricity:IN(0)", "electricity"] ]
    )
  end

  def test_support_lib_with_multiple_scenarios
    compare_support_lib_outputs(
      make_support_instance(multiple_scenarios_params),
      [
        {
          id: "mc_electricity",
          string: "[components.mc_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"mc_electricity_blue_sky\"\n"\
          "loads_by_scenario.s1 = \"mc_electricity_s1\"\n"\
          "loads_by_scenario.s2 = \"mc_electricity_s2\"\n",
        },
        {
          id: "other_electricity",
          string: "[components.other_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"other_electricity_blue_sky\"\n"\
          "loads_by_scenario.s1 = \"other_electricity_s1\"\n"\
          "loads_by_scenario.s2 = \"other_electricity_s2\"\n"
        },
        {
          id: "utility_electricity_bus",
          string: "[components.utility_electricity_bus]\n"\
          "type = \"muxer\"\n"\
          "stream = \"electricity\"\n"\
          "num_inflows = 1\n"\
          "num_outflows = 2\n"\
          "dispatch_strategy = \"in_order\"\n",
        },
        {
          id: "utility_electricity_source",
          string: "[components.utility_electricity_source]\n"\
          "type = \"source\"\n"\
          "outflow = \"electricity\"\n"
        }
      ],
      [ ["utility_electricity_source:OUT(0)", "utility_electricity_bus:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(0)", "mc_electricity:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(1)", "other_electricity:IN(0)", "electricity"] ]
    )
  end

  def test_support_lib_with_add_one_electric_generator_at_building_level
    compare_support_lib_outputs(
      make_support_instance(add_one_electric_generator_at_building_level_params),
      [
        {
          id: "mc_electricity",
          string: "[components.mc_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"mc_electricity_blue_sky\"\n"
        },
        {
          id: "mc_electricity_bus",
          string: "[components.mc_electricity_bus]\n"\
          "type = \"muxer\"\n"\
          "stream = \"electricity\"\n"\
          "num_inflows = 2\n"\
          "num_outflows = 1\n"\
          "dispatch_strategy = \"in_order\"\n"
        },
        {
          id: "mc_electric_generator",
          string: "[components.mc_electric_generator]\n"\
          "type = \"converter\"\n"\
          "inflow = \"natural_gas\"\n"\
          "outflow = \"electricity\"\n"\
          "lossflow = \"waste_heat\"\n"\
          "constant_efficiency = 0.42\n"
        },
        {
          id: "other_electricity",
          string: "[components.other_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"other_electricity_blue_sky\"\n"
        },
        {
          id: "utility_electricity_bus",
          string: "[components.utility_electricity_bus]\n"\
          "type = \"muxer\"\n"\
          "stream = \"electricity\"\n"\
          "num_inflows = 1\n"\
          "num_outflows = 2\n"\
          "dispatch_strategy = \"in_order\"\n",
        },
        {
          id: "utility_electricity_source",
          string: "[components.utility_electricity_source]\n"\
          "type = \"source\"\n"\
          "outflow = \"electricity\"\n"
        },
        {
          id: "utility_natural_gas_source",
          string: "[components.utility_natural_gas_source]\n"\
          "type = \"source\"\n"\
          "outflow = \"natural_gas\"\n"
        }
      ],
      [ ["utility_electricity_source:OUT(0)", "utility_electricity_bus:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(0)", "mc_electricity_bus:IN(0)", "electricity"],
        ["mc_electric_generator:OUT(0)", "mc_electricity_bus:IN(1)", "electricity"],
        ["mc_electricity_bus:OUT(0)", "mc_electricity:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(1)", "other_electricity:IN(0)", "electricity"],
        ["utility_natural_gas_source:OUT(0)", "mc_electric_generator:IN(0)", "natural_gas"],
      ],
      false
    )
  end

  def test_support_lib_with_only_one_building_with_electric_loads
    compare_support_lib_outputs(
      make_support_instance(only_one_building_with_electric_loads_params),
      [
        {
          id: "b1_electricity",
          string: "[components.b1_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"b1_electricity_blue_sky\"\n"
        },
        {
          id: "utility_electricity_source",
          string: "[components.utility_electricity_source]\n"\
          "type = \"source\"\n"\
          "outflow = \"electricity\"\n"
        },
      ],
      [ ["utility_electricity_source:OUT(0)", "b1_electricity:IN(0)", "electricity"],
      ],
      false
    )
  end

  def test_support_lib_with_one_building_has_thermal_storage
    compare_support_lib_outputs(
      make_support_instance(one_building_has_thermal_storage_params),
      [
        {
          id: "utility_electricity_source",
          string: "[components.utility_electricity_source]\n"\
          "type = \"source\"\n"\
          "outflow = \"electricity\"\n"
        },
        {
          id: "utility_heating_source",
          string: "[components.utility_heating_source]\n"\
          "type = \"source\"\n"\
          "outflow = \"heating\"\n"
        },
        {
          id: "utility_electricity_bus",
          string: "[components.utility_electricity_bus]\n"\
          "type = \"muxer\"\n"\
          "stream = \"electricity\"\n"\
          "num_inflows = 1\n"\
          "num_outflows = 2\n"\
          "dispatch_strategy = \"in_order\"\n"
        },
        {
          id: "mc_electricity",
          string: "[components.mc_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"mc_electricity_blue_sky\"\n"
        },
        {
          id: "other_electricity",
          string: "[components.other_electricity]\n"\
          "type = \"load\"\n"\
          "inflow = \"electricity\"\n"\
          "loads_by_scenario.blue_sky = \"other_electricity_blue_sky\"\n"
        },
        {
          id: "mc_heating",
          string: "[components.mc_heating]\n"\
          "type = \"load\"\n"\
          "inflow = \"heating\"\n"\
          "loads_by_scenario.blue_sky = \"mc_heating_blue_sky\"\n"
        },
        {
          id: "mc_thermal_storage",
          string: "[components.mc_thermal_storage]\n"\
          "type = \"store\"\n"\
          "outflow = \"heating\"\n"\
          "inflow = \"heating\"\n"\
          "capacity_unit = \"kWh\"\n"\
          "capacity = 100.0\n"\
          "max_inflow = 10.0\n"
        },
      ],
      [ 
        ["utility_electricity_source:OUT(0)", "utility_electricity_bus:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(0)", "mc_electricity:IN(0)", "electricity"],
        ["utility_electricity_bus:OUT(1)", "other_electricity:IN(0)", "electricity"],
        ["utility_heating_source:OUT(0)", "mc_thermal_storage:IN(0)", "heating"],
        ["mc_thermal_storage:OUT(0)", "mc_heating:IN(0)", "heating"],
      ],
      true
    )
  end
end
