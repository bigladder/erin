require 'set'

class Support
  ELECTRICITY = 'electricity'
  NATURAL_GAS = 'natural_gas'
  attr_reader :components, :connections, :loads, :load_ids
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
    _create_load_data
    _create_node_config
    _create_building_config
    @connections = []
    @components = []
    _add_electrical_connections_and_components
    _add_ng_connections_and_components
    _add_heating_connections_and_components
  end

  private

  def _create_load_data
    @loads = []
    @load_ids = []
    load_id_set = Set.new
    @load_profile_building_id.each_with_index do |b_id, n|
      s_id = @load_profile_scenario_id[n]
      enduse = @load_profile_enduse[n]
      file = @load_profile_file[n]
      load_id = "#{b_id}_#{enduse}_#{s_id}"
      if load_id_set.include?(load_id)
        raise "ERROR! Duplicate load_id \"#{load_id}\""
      end
      load_id_set << load_id
      @load_ids << load_id
      @loads << {load_id: load_id, file: file}
    end
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

  # INTERNAL METHOD
  def _enduses_for_building(id)
    enduses = Set.new
    @load_profile_building_id.each_with_index do |b_id, n|
      next unless b_id == id
      enduses << @load_profile_enduse[n]
    end
    enduses
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
        enduses: _enduses_for_building(b_id),
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

  def make_building_electrical_enduse(building_id)
    scenarios_string = ""
    id = "#{building_id}_#{ELECTRICITY}"
    s = "[components.#{building_id}_#{ELECTRICITY}]\n"\
      "type = \"load\"\n"\
      "inflow = \"#{ELECTRICITY}\"\n#{scenarios_string}"
    [id, {id: id, string: s}]
  end

  def make_building_electrical_bus(building_id)
    id = "#{building_id}_electrical_bus"
    [id, {id: id, string: ""}]
  end

  def _add_electrical_connections_and_components
    conns = []
    comps = {}
    #node_sources = {}
    @building_config.keys.sort.each_with_index do |b_id, n|
      cfg = @building_config[b_id]
      next unless cfg[:enduses].include?(ELECTRICITY)
      comp_id, comp = make_building_electrical_enduse(b_id)
      comps[comp_id] = comp
      if cfg[:has_egen]
        comp_id, comp = make_building_electrical_bus(b_id)
        comps[comp_id] = comp
        cfg[:enduses] << NATURAL_GAS # to fuel the electric generator
      end
    end
    @connections += conns.sort
    @components += comps.keys.sort.map {|k| comps[k]}
  end

  def _add_ng_connections_and_components
  end

  def  _add_heating_connections_and_components
  end
end
