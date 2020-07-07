require 'set'

class Support
  def initialize(load_profile_scenario_id,
                 load_profile_building_id,
                 load_profile_enduse,
                 load_profile_file,
                 # Building Level
                 building_level_building_id,
                 building_level_egen_flag,
                 building_level_egen_eff_pct,
                 building_level_heat_storage_flag,
                 building_level_heat_storage_cap_kWh,
                 building_level_gas_boiler_flag,
                 building_level_gas_boiler_eff_pct,
                 building_level_electricity_supply_node,
                 # Node Level
                 node_level_id,
                 node_level_ng_power_plant_flag,
                 node_level_ng_power_plant_eff_pct,
                 node_level_ng_supply_node
                )
    @load_profile_scenario_id = load_profile_scenario_id
    @load_profile_building_id = load_profile_building_id
    @load_profile_enduse = load_profile_enduse
    @load_profile_file = load_profile_file
    _check_load_profile_data
    @building_level_building_id = building_level_building_id
    @building_level_egen_flag = building_level_egen_flag
    @building_level_egen_eff_pct = building_level_egen_eff_pct
    #building_level_egen_peak_pwr_kW
    @building_level_heat_storage_flag = building_level_heat_storage_flag
    @building_level_heat_storage_cap_kWh = building_level_heat_storage_cap_kWh
    @building_level_gas_boiler_flag = building_level_gas_boiler_flag
    @building_level_gas_boiler_eff_pct = building_level_gas_boiler_eff_pct
    @building_level_electricity_supply_node = building_level_electricity_supply_node
    _check_building_level_data
    @node_level_id = node_level_id
    @node_level_ng_power_plant_flag = node_level_ng_power_plant_flag
    @node_level_ng_power_plant_eff_pct = node_level_ng_power_plant_eff_pct
    @node_level_ng_supply_node = node_level_ng_supply_node
    _check_node_level_data
    _create_node_config
    _create_building_config
  end

  # INTERNAL METHOD
  def _assert_lengths_the_same(array_of_arrays, msg)
    size = nil
    array_of_arrays.each do |xs|
      if size.nil?
        size = xs.size
      elsif size != xs.size
        raise "Array lengths not the same #{msg}"
      end
    end
  end

  # INTERNAL METHOD
  def _check_load_profile_data
    _assert_lengths_the_same(
      [
        @load_profile_scenario_id,
        @load_profile_building_id,
        @load_profile_enduse,
        @load_profile_file
      ],
      "load_profile_* data should all have the same length")
  end

  # INTERNAL METHOD
  def _check_building_level_data
    _assert_lengths_the_same(
      [
        @building_level_building_id,
        @building_level_egen_flag,
        @building_level_egen_eff_pct,
        @building_level_heat_storage_flag,
        @building_level_heat_storage_cap_kWh,
        @building_level_gas_boiler_flag,
        @building_level_gas_boiler_eff_pct,
        @building_level_electricity_supply_node
      ],
      "building_level_* data should all have the same length")
  end

  # INTERNAL METHOD
  def _check_node_level_data
    _assert_lengths_the_same(
      [
        @node_level_id,
        @node_level_ng_power_plant_flag,
        @node_level_ng_power_plant_eff_pct,
        @node_level_ng_supply_node
      ],
      "node_level_* data should all have the same length")
  end

  # INTERNAL METHOD
  def is_true(x)
    (x == true) || (x.to_s.downcase.strip == "true")
  end

  # INTERNAL METHOD
  def equals(x, y)
    (x.to_s.downcase.strip == y.to_s.downcase.strip)
  end

  # - building_ids: (Array string), array of building ids
  # - scenario_ids: (Array string), array of scenario ids
  # - enduses: (Array string), array of enduses
  # - load_profiles: (Array string), array of paths to csv files with the loads
  # RETURN: {
  #   :warnings => (Array string), any warnings found
  #   :loads => (Array {:load_id=>string, :file=>string}), the load data
  #   :load_ids => (Array string), the load ids used
  #   }
  def process_load_data
    loads = []
    warns = []
    load_id_set = Set.new
    load_ids = []
    @load_profile_building_id.each_with_index do |b_id, n|
      s_id = @load_profile_scenario_id[n]
      enduse = @load_profile_enduse[n]
      file = @load_profile_file[n]
      load_id = "#{b_id}_#{enduse}_#{s_id}"
      if load_id_set.include?(load_id)
        warns << "WARNING! Duplicate load_id \"#{load_id}\""
      end
      load_id_set << load_id
      load_ids << load_id
      loads << {load_id: load_id, file: file}
    end
    {
      warnings: warns,
      loads: loads,
      load_ids: load_ids,
    }
  end

  # INTERNAL METHOD
  def _create_building_config
    return if !@building_config.nil?
    @building_config = {}
    @building_level_building_id.each_with_index do |b_id, n|
      if @building_config.include?(b_id)
        raise "Building ID \"#{b_id}\" is multiply defined!"
      end
      @building_config[b_id] = {
        has_egen: is_true(@building_level_egen_flag[n]),
        egen_eff: @building_level_egen_eff_pct[n].to_f,
        has_tes: is_true(@building_level_heat_storage_flag[n]),
        tes_cap_kWh: @building_level_heat_storage_cap_kWh[n].to_f,
        has_boiler: is_true(@building_level_gas_boiler_flag[n]),
        boiler_eff: @building_level_gas_boiler_eff_pct[n].to_f / 100.0,
        e_supply_node: @building_level_electricity_supply_node[n].strip,
        ng_supply_node: "utility",
        enduses: Set.new,
      }
    end
  end

  # INTERNAL METHOD
  def _create_node_config
    return if !@node_config.nil?
    @node_config = {}
    @node_level_id.each_with_index do |n_id, n|
      if @node_config.include?(n_id)
        raise "node id \"#{n_id}\" multiply defined in node_config"
      end
      @node_config[n_id] = {
        has_ng_pwr_plant: is_true(@node_level_ng_power_plant_flag[n]),
        ng_pwr_plant_eff: @node_level_ng_power_plant_eff_pct[n].to_f / 100.0,
        ng_supply_node: @node_level_ng_supply_node[n].strip,
      }
    end
  end

  def get_connections
  end

  def get_components
  end
end
