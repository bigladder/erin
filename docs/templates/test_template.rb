#!/user/bin/env ruby
# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE file for additional terms and conditions.
######################################################################
# This is a unit test file for template.toml and various examples of exercising
# it. This module uses Ruby's standard unit test library, MiniTest This
# library assumes that Modelkit is installed and available from the commandline
require 'csv'
require 'fileutils'
require 'minitest/autorun'
require 'open3'
require 'set'
require 'stringio'

THIS_DIR = File.expand_path(File.dirname(__FILE__))
REMOVE_FILES = true

class TestTemplate < Minitest::Test
  # RETURN: (Hash symbol any), the default parameters for the template
  def default_params
    {
      :converter_component => [],
      :fixed_cdf => [
        {
          :id => "always",
          :value_in_hours => 0,
        },
      ],
      :general => {
        :simulation_duration_in_years => 100,
        :random_setting => "Auto",
        :random_seed => 17,
      },
      :load_component => [
        {
          :location_id => "mc",
          :inflow => "electricity",
        },
        {
          :location_id => "other",
          :inflow => "electricity",
        },
      ],
      :load_profile => [
        {
          :scenario_id => "blue_sky",
          :building_id => "mc",
          :enduse => "electricity",
          :file => "mc_blue_sky_electricity.csv",
        },
        {
          :scenario_id => "blue_sky",
          :building_id => "other",
          :enduse => "electricity",
          :file => "other_blue_sky_electricity.csv",
        },
      ],
      :scenario => [
        {
          :id => "blue_sky",
          :duration_in_hours => 8760,
          :occurrence_distribution => "always",
          :calc_reliability => false,
          :max_occurrence => 1,
        },
      ],
      :source_component => [
        {
          :location_id => "utility",
          :outflow => "electricity",
          :is_limited => "FALSE",
          :max_outflow_kW => 0.0,
        },
      ],
      :storage_component => [],
      :network_link => [
        {
          :source_location_id => "utility",
          :destination_location_id => "mc",
          :flow => "electricity",
        },
        {
          :source_location_id => "utility",
          :destination_location_id => "other",
          :flow => "electricity",
        },
      ],
    }
  end

  # RETURN: (Hash symbol any), the default parameters for the template
  def multiple_scenarios_params
    ps = default_params
    ps[:fixed_cdf] += [
      {
        :id => "fixed_10yrs",
        :value_in_hours => 8760 * 10,
      },
    ]
    ps[:load_profile] += [
      {
        :scenario_id => "s1",
        :building_id => "mc",
        :enduse => "electricity",
        :file => "mc_s1_electricity.csv",
      },
      {
        :scenario_id => "s1",
        :building_id => "other",
        :enduse => "electricity",
        :file => "other_s1_electricity.csv",
      },
      {
        :scenario_id => "s2",
        :building_id => "mc",
        :enduse => "electricity",
        :file => "mc_s2_electricity.csv",
      },
      {
        :scenario_id => "s2",
        :building_id => "other",
        :enduse => "electricity",
        :file => "other_s2_electricity.csv",
      },
    ]
    ps[:scenario] += [
      {
        :id => "s1",
        :duration_in_hours => 500,
        :occurrence_distribution => "fixed_10yrs",
        :calc_reliability => false,
        :max_occurrence => -1,
      },
      {
        :id => "s2",
        :duration_in_hours => 500,
        :occurrence_distribution => "fixed_10yrs",
        :calc_reliability => false,
        :max_occurrence => -1,
      },
    ]
    ps
  end

  def add_one_electric_generator_at_building_level_params
    ps = default_params
    ps[:converter_component] += [
      {
        :location_id => "mc",
        :inflow => "natural_gas",
        :outflow => "electricity",
        :lossflow => "waste_heat",
        :constant_efficiency => 0.42,
      },
    ]
    ps[:source_component] += [
      {
        :location_id => "utility",
        :outflow => "natural_gas",
        :is_limited => "FALSE",
        :max_outflow_kW => 0.0,
      },
    ]
    ps[:network_link] += [
      {
        :source_location_id => "utility",
        :destination_location_id => "mc",
        :flow => "natural_gas",
      },
    ]
    ps
  end

  def only_one_building_with_electric_loads_params
    ps = default_params
    ps[:load_profile] = [
      {
        :scenario_id => "blue_sky",
        :building_id => "b1",
        :enduse => "electricity",
        :file => "b1_blue_sky_electricity.csv",
      },
    ]
    ps[:load_component] = [
      {
        :location_id => "b1",
        :inflow => "electricity",
      },
    ]
    ps[:network_link] = [
      {
        :source_location_id => "utility",
        :destination_location_id => "b1",
        :flow => "electricity",
      },
    ]
    ps
  end

  def one_building_has_thermal_storage_params
    ps = default_params
    ps[:load_profile] << {
      :scenario_id => "blue_sky",
      :building_id => "mc",
      :enduse => "heating",
      :file => "mc_blue_sky_heating.csv",
    }
    ps[:load_component] << {
      :location_id => "mc",
      :inflow => "heating",
    }
    ps[:source_component] << {
      :location_id => "utility",
      :outflow => "heating",
      :is_limited => "FALSE",
      :max_outflow_kW => 0.0,
    }
    ps[:storage_component] << {
      :location_id => "mc",
      :flow => "heating",
      :capacity_kWh => 100.0,
      :max_inflow_kW => 10.0,
    }
    ps[:network_link] << {
      :source_location_id => "utility",
      :destination_location_id => "mc",
      :flow => "heating",
    }
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

  # - csv_path: string, path to csv file to write
  # - headers: (array symbol), the column headers to write
  # - data: (Hash symbol any), the data to pull headers from. Data having an
  #     entry symbol in headers must be an array of object convertable to
  #     string of the same length
  # - new_headers: (or nil (Array string)), the new headers to write
  # NOTE: checks that all headers are in data and that
  #       all arrays at data[header[n]] have the same length
  # RETURN: nil
  def write_csv(csv_path, headers, data)
    File.open(csv_path, 'w') do |f|
      csv = CSV.new(f)
      csv << headers.map(&:to_s)
      data.each do |row_data|
        row = []
        headers.each {|h| row << row_data[h].to_s}
        csv << row
      end
    end
    nil
  end

  # - csv_path: string, path to csv file to write
  # - headers: (array symbol), the column headers to write
  # - data: (Hash symbol any), the data to pull headers from. All data must be
  #     convertable to a string
  # NOTE: checks that all headers are in data
  # RETURN: nil
  def write_key_value_csv(csv_path, headers, data)
    headers.each do |h|
      if !data.include?(h)
        raise "ERROR: data does not include header \"#{h}\""
      end
    end
    File.open(csv_path, 'w') do |f|
      csv = CSV.new(f)
      csv << headers.map(&:to_s)
      row = []
      headers.each do |h|
        row << data[h].to_s
      end
      csv << row
    end
    nil
  end

  # - tmp_file: string, the template file path for the template to compose
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
  def call_modelkit(tmp_file, output_file)
    File.write(@params_file, ":expand_load_paths => false,")
    args = "--output=\"#{output_file}\" --file=\"#{@params_file}\" \"#{tmp_file}\""
    cmd = "modelkit template-compose #{args}"
    val = {}
    out = ""
    err = ""
    `#{cmd}`
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
    val[:csv_files] = ""
    @all_csvs.each do |path|
      val[:csv_files] += ("="*30 + path + "="*30 + "\n")
      if File.exist?(path)
        val[:csv_files] += File.read(path)
      else
        val[:csv_files] += "doesn't exist..."
      end
    end
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
  # - ensures generated files are removed if exist
  def ensure_directory_clean
    all_paths = [@params_file, @output_file] + @all_csvs
    all_paths.each do |path|
      File.delete(path) if File.exist?(path) and REMOVE_FILES
    end
  end

  def setup
    @params_file = "params.pxt"
    @template_file = "template.toml"
    @output_file = "test.toml"
    @converter_component_csv = "converter-component.csv"
    @fixed_cdf_csv = "fixed-cdf.csv"
    @general_csv = "general.csv"
    @load_component_csv = "load-component.csv"
    @load_profile_csv = "load-profile.csv"
    @network_link_csv = "network-link.csv"
    @scenario_csv = "scenario.csv"
    @source_component_csv = "source-component.csv"
    @storage_component_csv = "storage-component.csv"
    @all_csvs = [
      @converter_component_csv,
      @fixed_cdf_csv,
      @general_csv,
      @load_component_csv,
      @load_profile_csv,
      @network_link_csv,
      @scenario_csv,
      @source_component_csv,
      @storage_component_csv,
    ]
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
  def run_and_compare(data, reference_tag, save_rendered_template = true)
    info = [
      [
        @converter_component_csv,
        [:location_id, :inflow, :outflow, :lossflow, :constant_efficiency],
        :converter_component,
        :normal_table,
      ],
      [
        @fixed_cdf_csv,
        [:id, :value_in_hours],
        :fixed_cdf,
        :normal_table,
      ],
      [
        @general_csv, 
        [:simulation_duration_in_years, :random_setting, :random_seed],
        :general,
        :kv_table,
      ],
      [
        @load_component_csv,
        [:location_id, :inflow],
        :load_component,
        :normal_table,
      ],
      [
        @load_profile_csv,
        [:scenario_id, :building_id, :enduse, :file],
        :load_profile,
        :normal_table,
      ],
      [
        @network_link_csv,
        [:source_location_id, :destination_location_id, :flow],
        :network_link,
        :normal_table,
      ],
      [
        @scenario_csv,
        [:id, :duration_in_hours, :occurrence_distribution,
         :calc_reliability, :max_occurrence],
        :scenario,
        :normal_table,
      ],
      [
        @source_component_csv,
        [:location_id, :outflow, :is_limited, :max_outflow_kW],
        :source_component,
        :normal_table,
      ],
      [
        @storage_component_csv,
        [:location_id, :flow, :capacity_kWh, :max_inflow_kW],
        :storage_component,
        :normal_table,
      ],
    ]
    info.each do |path, header, key, table_type|
      if table_type == :kv_table
        write_key_value_csv(
          path,
          header,
          data.fetch(key),
        )
      elsif table_type == :normal_table
        write_csv(
          path,
          header,
          data.fetch(key),
        )
      else
        raise "unhandled table type '#{table_type}'"
      end
    end
    out = call_modelkit(@template_file, @output_file)
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
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_e2rin('defaults', load_profiles))
    assert(run_e2rin_graph('defaults', load_profiles))
  end

  def test_multiple_scenarios
    ps = multiple_scenarios_params
    run_and_compare(ps, 'multiple_scenarios')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_e2rin('multiple_scenarios', load_profiles))
    assert(run_e2rin_graph('multiple_scenarios', load_profiles))
  end

  def test_add_one_electric_generator_at_building_level
    ps = add_one_electric_generator_at_building_level_params
    run_and_compare(ps, 'add_one_electric_generator_at_building_level')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_e2rin(
        'add_one_electric_generator_at_building_level',
        load_profiles))
    assert(
      run_e2rin_graph(
        'add_one_electric_generator_at_building_level',
        load_profiles))
  end

  def test_only_one_building_with_electric_loads
    ps = only_one_building_with_electric_loads_params
    run_and_compare(ps, 'only_one_building_with_electric_loads')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_e2rin(
        'only_one_building_with_electric_loads', load_profiles))
    assert(
      run_e2rin_graph(
        'only_one_building_with_electric_loads', load_profiles))
  end

  def test_one_building_has_thermal_storage
    ps = one_building_has_thermal_storage_params
    run_and_compare(ps, 'one_building_has_thermal_storage')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_e2rin(
        'one_building_has_thermal_storage', load_profiles))
    assert(
      run_e2rin_graph(
        'one_building_has_thermal_storage', load_profiles))
  end

  def test_one_building_has_boiler
    ps = default_params
    ps[:converter_component] << {
      :location_id => "mc",
      :inflow => "natural_gas",
      :outflow => "heating",
      :lossflow => "waste_heat",
      :constant_efficiency => 0.85,
    }
    ps[:load_component] << {
      :location_id => "mc",
      :inflow => "heating",
    }
    ps[:load_profile] << {
      :scenario_id => "blue_sky",
      :building_id => "mc",
      :enduse => "heating",
      :file => "mc_blue_sky_heating.csv",
    }
    ps[:network_link] += [
      {
        :source_location_id => "utility",
        :destination_location_id => "mc",
        :flow => "heating",
      },
      {
        :source_location_id => "utility",
        :destination_location_id => "mc",
        :flow => "natural_gas",
      },
    ]
    ps[:source_component] += [
      {
        :location_id => "utility",
        :outflow => "heating",
        :is_limited => "FALSE",
        :max_outflow_kW => 0.0,
      },
      {
        :location_id => "utility",
        :outflow => "natural_gas",
        :is_limited => "FALSE",
        :max_outflow_kW => 0.0,
      },
    ]
    run_and_compare(ps, 'one_building_has_boiler')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_e2rin(
        'one_building_has_boiler', load_profiles))
    assert(
      run_e2rin_graph(
        'one_building_has_boiler', load_profiles))
  end

  def test_all_building_config_options_true
    ps = {
      :converter_component => [
        {
          :location_id => "mc",
          :inflow => "natural_gas",
          :outflow => "electricity",
          :lossflow => "waste_heat",
          :constant_efficiency => 0.32,
        },
        {
          :location_id => "other",
          :inflow => "natural_gas",
          :outflow => "electricity",
          :lossflow => "waste_heat",
          :constant_efficiency => 0.32,
        },
        {
          :location_id => "mc",
          :inflow => "natural_gas",
          :outflow => "heating",
          :lossflow => "waste_heat",
          :constant_efficiency => 0.85,
        },
        {
          :location_id => "other",
          :inflow => "natural_gas",
          :outflow => "heating",
          :lossflow => "waste_heat",
          :constant_efficiency => 0.85,
        },
      ],
      :fixed_cdf => [
        {
          :id => "always",
          :value_in_hours => 0,
        },
      ],
      :general => {
        :simulation_duration_in_years => 100,
        :random_setting => "Auto",
        :random_seed => 17,
      },
      :load_component => [
        {
          :location_id => "mc",
          :inflow => "electricity",
        },
        {
          :location_id => "other",
          :inflow => "electricity",
        },
        {
          :location_id => "mc",
          :inflow => "heating",
        },
        {
          :location_id => "other",
          :inflow => "heating",
        },
      ],
      :load_profile => [
        {
          :scenario_id => "blue_sky",
          :building_id => "mc",
          :enduse => "electricity",
          :file => "mc_blue_sky_electricity.csv",
        },
        {
          :scenario_id => "blue_sky",
          :building_id => "other",
          :enduse => "electricity",
          :file => "other_blue_sky_electricity.csv",
        },
        {
          :scenario_id => "blue_sky",
          :building_id => "mc",
          :enduse => "heating",
          :file => "mc_blue_sky_heating.csv",
        },
        {
          :scenario_id => "blue_sky",
          :building_id => "other",
          :enduse => "heating",
          :file => "other_blue_sky_heating.csv",
        },
      ],
      :network_link => [
        {
          :source_location_id => "utility",
          :destination_location_id => "mc",
          :flow => "electricity",
        },
        {
          :source_location_id => "utility",
          :destination_location_id => "other",
          :flow => "electricity",
        },
        {
          :source_location_id => "utility",
          :destination_location_id => "mc",
          :flow => "natural_gas",
        },
        {
          :source_location_id => "utility",
          :destination_location_id => "other",
          :flow => "natural_gas",
        },
        {
          :source_location_id => "utility",
          :destination_location_id => "mc",
          :flow => "heating",
        },
        {
          :source_location_id => "utility",
          :destination_location_id => "other",
          :flow => "heating",
        },
      ],
      :scenario => [
        {
          :id => "blue_sky",
          :duration_in_hours => 8760,
          :occurrence_distribution => "always",
          :calc_reliability => false,
          :max_occurrence => 1,
        },
      ],
      :source_component => [
        {
          :location_id => "utility",
          :outflow => "electricity",
          :is_limited => "FALSE",
          :max_outflow_kW => 0.0,
        },
        {
          :location_id => "utility",
          :outflow => "natural_gas",
          :is_limited => "FALSE",
          :max_outflow_kW => 0.0,
        },
        {
          :location_id => "utility",
          :outflow => "heating",
          :is_limited => "FALSE",
          :max_outflow_kW => 0.0,
        },
      ],
      :storage_component => [
        {
          :location_id => "mc",
          :flow => "heating",
          :capacity_kWh => 100.0,
          :max_inflow_kW => 10.0,
        },
        {
          :location_id => "other",
          :flow => "heating",
          :capacity_kWh => 100.0,
          :max_inflow_kW => 10.0,
        },
      ],
    }
    run_and_compare(ps, 'all_building_config_options_true')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_e2rin(
        'all_building_config_options_true', load_profiles))
    assert(
      run_e2rin_graph(
        'all_building_config_options_true', load_profiles))
  end
end
