#!/user/bin/env ruby
# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE file for additional terms and conditions.
######################################################################
# This is a unit test file for template.toml and various examples of exercising it.
# This module uses Ruby's standard unit test library, MiniTest
# This library assumes that Modelkit is installed and available from the
# commandline
require 'minitest/autorun'
require 'open3'
require 'stringio'

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
      # Building-to-Cluster Connectivity
      # Cluster-to-Community Connectivity
      # Community-to-Utility Connectivity
      # Building Level Configuration
      :building_level_building_id => ["mc", "other"],
      :building_level_egen_flag => ["FALSE", "FALSE"],
      :building_level_egen_eff_pct => [32.0, 32.0],
      #:building_level_egen_peak_pwr_kW => [100.0, 100.0],
      :building_level_heat_storage_flag => ["FALSE", "FALSE"],
      :building_level_heat_storage_cap_kWh => [0.0, 0.0],
      :building_level_gas_boiler_flag => ["FALSE", "FALSE"],
      :building_level_gas_boiler_eff_pct => [85.0, 85.0],
      #:building_level_gas_boiler_peak_heat_gen_kW => [50.0, 50.0],
      #:building_level_echiller_flag => ["FALSE", "FALSE"],
      #:building_level_echiller_peak_cooling_kW => [50.0, 50.0],
      # Community Level Configuration
    }
  end

  # RETURN: string, path to the e2rin_multi executable
  def e2rin_path
    path1 = File.join(THIS_DIR, '..', '..', 'build', 'bin', 'e2rin_multi')
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
  def run_and_compare(params, reference_tag)
    write_params(params, @params_file)
    out = call_modelkit(@params_file, @template_file, @output_file)
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
  end

  def test_multiple_scenarios
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
    run_and_compare(ps, 'multiple_scenarios')
    assert(run_e2rin('multiple_scenarios', ps[:load_profile_file]))
  end

  def test_add_one_electric_generator_at_building_level
    ps = default_params
    ps[:building_level_egen_flag][0] = "TRUE"
    ps[:building_level_egen_eff_pct][0] = 42.0
    run_and_compare(ps, 'add_one_electric_generator_at_building_level')
    assert(run_e2rin('add_one_electric_generator_at_building_level', ps[:load_profile_file]))
  end

  def test_only_one_building_with_electric_loads
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
    run_and_compare(ps, 'only_one_building_with_electric_loads')
    assert(run_e2rin('only_one_building_with_electric_loads', ps[:load_profile_file]))
  end
end
