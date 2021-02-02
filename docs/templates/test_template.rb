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

  def most_basic_params
    {
      :component_failure_mode => [],
      :component_fragility => [],
      :converter_component => [],
      :damage_intensity => [],
      :dual_outflow_converter_comp => [],
      :failure_mode => [],
      :dist_type => [
        {
          :id => "every_30_years",
          :dist_type => "fixed",
        },
      ],
      :fixed_dist => [
        {
          :id => "every_30_years",
          :value_in_hours => 8760*30,
        },
      ],
      :uniform_dist => [],
      :normal_dist => [],
      :weibull_dist => [],
      :quantile_dist => [],
      :fragility_curve => [],
      :general => {
        :simulation_duration_in_years => 100,
        :random_setting => "Auto",
        :random_seed => 17,
      },
      :load_component => [
        {
          :id => "b1_electricity",
          :location_id => "b1",
          :inflow => "electricity",
        },
      ],
      :load_profile => [
        {
          :scenario_id => "s1",
          :component_id => "b1_electricity",
          :flow => "electricity",
          :file => "s1_b1_electricity.csv",
        },
      ],
      :mover_component => [],
      :pass_through_component => [],
      :scenario => [
        {
          :id => "s1",
          :duration_in_hours => 14*24,
          :occurrence_distribution => "every_30_years",
          :calc_reliability => false,
          :max_occurrence => -1,
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
          :destination_location_id => "b1",
          :flow => "electricity",
        },
      ],
      :uncontrolled_src => [],
    }
  end

  # RETURN: (Hash symbol any), the default parameters for the template
  def default_params
    {
      :component_failure_mode => [],
      :component_fragility => [],
      :converter_component => [],
      :damage_intensity => [],
      :dual_outflow_converter_comp => [],
      :failure_mode => [],
      :dist_type => [
        {
          :id => "always",
          :dist_type => "fixed",
        },
      ],
      :fixed_dist => [
        {
          :id => "always",
          :value_in_hours => 0,
        },
      ],
      :uniform_dist => [],
      :normal_dist => [],
      :weibull_dist => [],
      :quantile_dist => [],
      :fragility_curve => [],
      :general => {
        :simulation_duration_in_years => 100,
        :random_setting => "Auto",
        :random_seed => 17,
      },
      :load_component => [
        {
          :id => "mc_electricity",
          :location_id => "mc",
          :inflow => "electricity",
        },
        {
          :id => "other_electricity",
          :location_id => "other",
          :inflow => "electricity",
        },
      ],
      :load_profile => [
        {
          :scenario_id => "blue_sky",
          :component_id => "mc_electricity",
          :flow => "electricity",
          :file => "mc_blue_sky_electricity.csv",
        },
        {
          :scenario_id => "blue_sky",
          :component_id => "other_electricity",
          :flow => "electricity",
          :file => "other_blue_sky_electricity.csv",
        },
      ],
      :mover_component => [],
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
      :pass_through_component => [],
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
      :uncontrolled_src => [],
    }
  end

  # RETURN: (Hash symbol any), the default parameters for the template
  def multiple_scenarios_params
    ps = default_params
    ps[:dist_type] += [
      {
        :id => "fixed_10yrs",
        :dist_type => "fixed",
      },
    ]
    ps[:fixed_dist] += [
      {
        :id => "fixed_10yrs",
        :value_in_hours => 8760 * 10,
      },
    ]
    ps[:load_profile] += [
      {
        :scenario_id => "s1",
        :component_id => "mc_electricity",
        :flow => "electricity",
        :file => "mc_s1_electricity.csv",
      },
      {
        :scenario_id => "s1",
        :component_id => "other_electricity",
        :flow => "electricity",
        :file => "other_s1_electricity.csv",
      },
      {
        :scenario_id => "s2",
        :component_id => "mc_electricity",
        :flow => "electricity",
        :file => "mc_s2_electricity.csv",
      },
      {
        :scenario_id => "s2",
        :component_id => "other_electricity",
        :flow => "electricity",
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
        :component_id => "b1_electricity",
        :flow => "electricity",
        :file => "b1_blue_sky_electricity.csv",
      },
    ]
    ps[:load_component] = [
      {
        :id => "b1_electricity",
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
      :component_id => "mc_heating",
      :flow => "heating",
      :file => "mc_blue_sky_heating.csv",
    }
    ps[:load_component] << {
      :id => "mc_heating",
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

  # RETURN: string, path to the erin_multi executable
  def erin_path
    path1 = File.join(THIS_DIR, '..', '..', 'build', 'bin', 'erin_multi')
    path2 = File.join(THIS_DIR, '..', '..', 'build', 'bin', 'Debug', 'erin_multi.exe')
    if File.exist?(path1)
      path1
    elsif File.exist?(path2)
      path2
    else
      throw "Could not find path to erin_multi"
    end
  end

  def erin_graph_path
    path1 = File.join(THIS_DIR, '..', '..', 'build', 'bin', 'erin_graph')
    path2 = File.join(THIS_DIR, '..', '..', 'build', 'bin', 'Debug', 'erin_graph.exe')
    if File.exist?(path1)
      path1
    elsif File.exist?(path2)
      path2
    else
      throw "Could not find path to erin_graph"
    end
  end

  # - input_tag: string, the tag for the input file, such as 'defaults', for reference/defaults.toml
  # - load_profiles: (array string), the csv files for the load profiles
  # RETURN: bool
  def run_erin(input_tag, load_profiles)
    load_profiles.each do |path|
      File.open(path, 'w') do |f|
        f.write("hours,kW\n")
        f.write("0,1\n")
        f.write("10000,0\n")
      end
    end
    input_path = File.join(THIS_DIR, 'reference', input_tag + ".toml")
    `#{erin_path} #{input_path} out.csv stats.csv`
    success = ($?.to_i == 0)
    (load_profiles + ['out.csv', 'stats.csv']).each do |path|
      File.delete(path) if File.exist?(path)
    end
    return success
  end

  # - input_tag: string, the tag for the input file, such as 'defaults', for reference/defaults.toml
  # - load_profiles: (array string), the csv files for the load profiles
  # RETURN: bool
  def run_erin_graph(input_tag, load_profiles)
    load_profiles.each do |path|
      File.open(path, 'w') do |f|
        f.write("hours,kW\n")
        f.write("0,1\n")
        f.write("10000,0\n")
      end
    end
    input_path = File.join(THIS_DIR, 'reference', input_tag + ".toml")
    `#{erin_graph_path} #{input_path} #{input_tag}.gv nw`
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
    val[:params] = if File.exist?(@params_file) then
                     File.read(@params_file)
                   else
                     ""
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
    # ------------ CSV Files -------------------------------
    @component_failure_mode_csv = "component-failure-mode.csv"
    @component_fragility_csv = "component-fragility.csv"
    @converter_component_csv = "converter-component.csv"
    @damage_intensity_csv = "damage-intensity.csv"
    @dual_outflow_converter_comp_csv = "dual-outflow-converter-comp.csv"
    @failure_mode_csv = "failure-mode.csv"
    @dist_type_csv = "dist-type.csv"
    @fixed_dist_csv = "fixed-dist.csv"
    @uniform_dist_csv = "uniform-dist.csv"
    @normal_dist_csv = "normal-dist.csv"
    @weibull_dist_csv = "weibull-dist.csv"
    @quantile_dist_csv = "quantile-dist.csv"
    @fragility_curve_csv = "fragility-curve.csv"
    @general_csv = "general.csv"
    @load_component_csv = "load-component.csv"
    @load_profile_csv = "load-profile.csv"
    @mover_component_csv = "mover-component.csv"
    @network_link_csv = "network-link.csv"
    @pass_through_component_csv = "pass-through-component.csv"
    @scenario_csv = "scenario.csv"
    @source_component_csv = "source-component.csv"
    @storage_component_csv = "storage-component.csv"
    @uncontrolled_src_csv = "uncontrolled-src.csv"
    @all_csvs = [
      @component_failure_mode_csv,
      @component_fragility_csv,
      @converter_component_csv,
      @damage_intensity_csv,
      @dual_outflow_converter_comp_csv,
      @failure_mode_csv,
      @dist_type_csv,
      @fixed_dist_csv,
      @uniform_dist_csv,
      @normal_dist_csv,
      @weibull_dist_csv,
      @quantile_dist_csv,
      @fragility_curve_csv,
      @general_csv,
      @load_component_csv,
      @load_profile_csv,
      @mover_component_csv,
      @network_link_csv,
      @pass_through_component_csv,
      @scenario_csv,
      @source_component_csv,
      @storage_component_csv,
      @uncontrolled_src_csv,
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
        @component_failure_mode_csv,
        [:component_id, :failure_mode_id],
        :component_failure_mode,
        :normal_table,
      ],
      [
        @component_fragility_csv,
        [:component_id, :fragility_id],
        :component_fragility,
        :normal_table,
      ],
      [
        @converter_component_csv,
        [:location_id, :inflow, :outflow, :lossflow, :constant_efficiency],
        :converter_component,
        :normal_table,
      ],
      [
        @damage_intensity_csv,
        [:scenario_id, :name, :value],
        :damage_intensity,
        :normal_table,
      ],
      [
        @dual_outflow_converter_comp_csv,
        [
          :location_id, :inflow, :primary_outflow,
          :secondary_outflow, :lossflow, :primary_efficiency,
          :secondary_efficiency,
        ],
        :dual_outflow_converter_comp,
        :normal_table,
      ],
      [
        @failure_mode_csv,
        [:id, :failure_dist, :repair_dist],
        :failure_mode,
        :normal_table,
      ],
      [
        @dist_type_csv,
        [:id, :dist_type],
        :dist_type,
        :normal_table,
      ],
      [
        @fixed_dist_csv,
        [:id, :value_in_hours],
        :fixed_dist,
        :normal_table,
      ],
      [
        @uniform_dist_csv,
        [:id, :lower_bound_in_hours, :upper_bound_in_hours],
        :uniform_dist,
        :normal_table,
      ],
      [
        @normal_dist_csv,
        [:id, :mean_in_hours, :standard_deviation_in_hours],
        :normal_dist,
        :normal_table,
      ],
      [
        @weibull_dist_csv,
        [:id, :shape, :scale_in_hours, :location_in_hours],
        :weibull_dist,
        :normal_table,
      ],
      [
        @quantile_dist_csv,
        [:id, :csv_file],
        :quantile_dist,
        :normal_table,
      ],
      [
        @fragility_curve_csv,
        [:id, :vulnerable_to, :lower_bound, :upper_bound],
        :fragility_curve,
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
        [:scenario_id, :component_id, :flow, :file],
        :load_profile,
        :normal_table,
      ],
      [
        @mover_component_csv,
        [:location_id, :flow_moved, :support_flow, :cop],
        :mover_component,
        :normal_table,
      ],
      [
        @network_link_csv,
        [:source_location_id, :destination_location_id, :flow],
        :network_link,
        :normal_table,
      ],
      [
        @pass_through_component_csv,
        [:id, :link_id, :flow],
        :pass_through_component,
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
      [
        @uncontrolled_src_csv,
        [:location_id, :outflow],
        :uncontrolled_src,
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
    assert(run_erin('defaults', load_profiles))
    assert(run_erin_graph('defaults', load_profiles))
  end

  def test_multiple_scenarios
    ps = multiple_scenarios_params
    run_and_compare(ps, 'multiple_scenarios')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_erin('multiple_scenarios', load_profiles))
    assert(run_erin_graph('multiple_scenarios', load_profiles))
  end

  def test_add_one_electric_generator_at_building_level
    ps = add_one_electric_generator_at_building_level_params
    run_and_compare(ps, 'add_one_electric_generator_at_building_level')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_erin(
        'add_one_electric_generator_at_building_level',
        load_profiles))
    assert(
      run_erin_graph(
        'add_one_electric_generator_at_building_level',
        load_profiles))
  end

  def test_only_one_building_with_electric_loads
    ps = only_one_building_with_electric_loads_params
    run_and_compare(ps, 'only_one_building_with_electric_loads')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_erin(
        'only_one_building_with_electric_loads', load_profiles))
    assert(
      run_erin_graph(
        'only_one_building_with_electric_loads', load_profiles))
  end

  def test_one_building_has_thermal_storage
    ps = one_building_has_thermal_storage_params
    run_and_compare(ps, 'one_building_has_thermal_storage')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_erin(
        'one_building_has_thermal_storage', load_profiles))
    assert(
      run_erin_graph(
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
      :id => "mc_heating",
      :location_id => "mc",
      :inflow => "heating",
    }
    ps[:load_profile] << {
      :scenario_id => "blue_sky",
      :component_id => "mc_heating",
      :flow => "heating",
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
      run_erin(
        'one_building_has_boiler', load_profiles))
    assert(
      run_erin_graph(
        'one_building_has_boiler', load_profiles))
  end

  def test_all_building_config_options_true
    ps = {
      :component_failure_mode => [],
      :component_fragility => [],
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
      :damage_intensity => [],
      :dual_outflow_converter_comp => [],
      :failure_mode => [],
      :dist_type => [
        {
          :id => "always",
          :dist_type => "fixed",
        },
      ],
      :fixed_dist => [
        {
          :id => "always",
          :value_in_hours => 0,
        },
      ],
      :uniform_dist => [],
      :normal_dist => [],
      :weibull_dist => [],
      :quantile_dist => [],
      :fragility_curve => [],
      :general => {
        :simulation_duration_in_years => 100,
        :random_setting => "Auto",
        :random_seed => 17,
      },
      :load_component => [
        {
          :id => "mc_electricity",
          :location_id => "mc",
          :inflow => "electricity",
        },
        {
          :id => "other_electricity",
          :location_id => "other",
          :inflow => "electricity",
        },
        {
          :id => "mc_heating",
          :location_id => "mc",
          :inflow => "heating",
        },
        {
          :id => "other_heating",
          :location_id => "other",
          :inflow => "heating",
        },
      ],
      :load_profile => [
        {
          :scenario_id => "blue_sky",
          :component_id => "mc_electricity",
          :flow => "electricity",
          :file => "mc_blue_sky_electricity.csv",
        },
        {
          :scenario_id => "blue_sky",
          :component_id => "other_electricity",
          :flow => "electricity",
          :file => "other_blue_sky_electricity.csv",
        },
        {
          :scenario_id => "blue_sky",
          :component_id => "mc_heating",
          :flow => "heating",
          :file => "mc_blue_sky_heating.csv",
        },
        {
          :scenario_id => "blue_sky",
          :component_id => "other_heating",
          :flow => "heating",
          :file => "other_blue_sky_heating.csv",
        },
      ],
      :mover_component => [],
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
      :pass_through_component => [],
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
      :uncontrolled_src => [],
    }
    run_and_compare(ps, 'all_building_config_options_true')
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(
      run_erin(
        'all_building_config_options_true', load_profiles))
    assert(
      run_erin_graph(
        'all_building_config_options_true', load_profiles))
  end

  def test_add_damage_intensities_to_scenario
    ps = most_basic_params
    ps[:damage_intensity] = [
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
    ]
    tag = 'add_damage_intensities_to_scenario'
    run_and_compare(ps, tag)
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_erin(tag, load_profiles))
    assert(run_erin_graph(tag, load_profiles))
  end

  def test_add_fragility_curves_and_damage_intensity_to_scenarios
    ps = most_basic_params
    ps[:component_fragility] = [
      {
        component_id: "utility_electricity_source",
        fragility_id: "highly_vulnerable_to_wind",
      },
    ]
    ps[:damage_intensity] = [
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
    ]
    ps[:fragility_curve] = [
      {
        id: "highly_vulnerable_to_wind",
        vulnerable_to: "wind_speed_mph",
        lower_bound: 80.0,
        upper_bound: 160.0,
      },
    ]
    tag = 'add_fragility_curves_and_damage_intensity_to_scenarios'
    run_and_compare(ps, tag)
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_erin(tag, load_profiles))
    assert(run_erin_graph(tag, load_profiles))
  end

  def test_add_multiple_pass_through_components_on_a_link
    ps = most_basic_params
    ps[:pass_through_component] = [
      {
        id: "electric_lines_A",
        link_id: "utility_to_b1_electricity",
        flow: "electricity",
      },
      {
        id: "electric_lines_B",
        link_id: "utility_to_b1_electricity",
        flow: "electricity",
      },
    ]
    tag = 'add_multiple_pass_through_components_on_a_link'
    run_and_compare(ps, tag)
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_erin(tag, load_profiles))
    assert(run_erin_graph(tag, load_profiles))
  end

  def test_add_failure_modes_to_a_component
    ps = most_basic_params
    ps[:component_failure_mode] += [
      {
        component_id: "utility_electricity_source",
        failure_mode_id: "typical_utility_failures",
      },
    ]
    ps[:failure_mode] += [
      {
        id: "typical_utility_failures",
        failure_dist: "every_10_hours",
        repair_dist: "every_4_years",
      },
    ]
    ps[:dist_type] += [
      {
        id: "every_10_hours",
        dist_type: "fixed",
      },
      {
        id: "every_4_years",
        dist_type: "fixed",
      },
    ]
    ps[:fixed_dist] += [
      {
        id: "every_10_hours",
        value_in_hours: 10,
      },
      {
        id: "every_4_years",
        value_in_hours: 8760*4,
      },
    ]
    tag = 'add_failure_modes_to_a_component'
    run_and_compare(ps, tag)
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_erin(tag, load_profiles))
    assert(run_erin_graph(tag, load_profiles))
  end

  def test_add_chp
    ps = most_basic_params
    ps[:dual_outflow_converter_comp] = [
      {
        location_id: "b1",
        inflow: "natural_gas",
        primary_outflow: "electricity",
        secondary_outflow: "heating",
        lossflow: "waste_heat",
        primary_efficiency: 0.4,
        secondary_efficiency: 0.85,
      },
    ]
    ps[:load_component] << {
      id: "b1_heating",
      location_id: "b1",
      inflow: "heating",
    }
    ps[:load_profile] << {
      scenario_id: "s1",
      component_id: "b1_heating",
      flow: "heating",
      file: "s1_b1_heating.csv",
    }
    ps[:source_component] = [
      {
        location_id: "utility",
        outflow: "natural_gas",
        is_limited: "FALSE",
        max_outflow_kW: 0.0,
      },
    ]
    ps[:network_link] = [
      {
        source_location_id: "utility",
        destination_location_id: "b1",
        flow: "natural_gas",
      },
    ]
    tag = 'add_chp'
    run_and_compare(ps, tag)
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_erin(tag, load_profiles))
    assert(run_erin_graph(tag, load_profiles))
  end

  def test_add_cooling_load_and_chiller
    ps = most_basic_params
    ps[:load_component] = [
      {
        id: "environment_heat_sink",
        location_id: "environment",
        inflow: "heat_removed",
      },
    ]
    ps[:uncontrolled_src] = [
      {
        location_id: "b1",
        outflow: "heat_removed",
      },
    ]
    ps[:load_profile] = [
      {
        scenario_id: "s1",
        component_id: "b1_heat_removed_uncontrolled",
        flow: "heat_removed",
        file: "s1_b1_cooling.csv",
      },
      {
        scenario_id: "s1",
        component_id: "environment_heat_removed",
        flow: "heat_removed",
        file: "s1_environment_heat_sink.csv",
      },
    ]
    ps[:source_component] = [
      {
        location_id: "utility",
        outflow: "electricity",
        is_limited: "FALSE",
        max_outflow_kW: 0.0,
      },
    ]
    ps[:mover_component] = [
      {
        location_id: "b1",
        flow_moved: "heat_removed",
        support_flow: "electricity",
        cop: 5.0,
      },
    ]
    ps[:network_link] = [
      {
        source_location_id: "utility",
        destination_location_id: "b1",
        flow: "electricity",
      },
      {
        source_location_id: "b1",
        destination_location_id: "environment",
        flow: "heat_removed",
      },
    ]
    tag = 'add_cooling_load_and_chiller'
    run_and_compare(ps, tag)
    load_profiles = ps[:load_profile].map {|p| p[:file]}
    assert(run_erin(tag, load_profiles))
    assert(run_erin_graph(tag, load_profiles))
  end
end
