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

  def make_scenario_string(building_id, enduse)
    strings = []
    @building_level_building_id.each_with_index do |b_id, n|
      next unless building_id == b_id
      @load_profile_scenario_id.each_with_index do |s_id, nn|
        next unless @load_profile_building_id[nn] == b_id
        next unless @load_profile_enduse[nn] == enduse
        load_id = @load_ids[nn]
        strings << "loads_by_scenario.#{ s_id } = \"#{ load_id }\""
      end
    end
    strings.join("\n")
  end

  def add_if_not_added(dict, id, string)
    if !dict.include?(id)
      value = {id: id, string: string}
      dict[id] = value
      true
    else
      false
    end
  end

  def add_building_electrical_enduse(building_id, comps)
    scenarios_string = make_scenario_string(building_id, ELECTRICITY)
    id = "#{building_id}_#{ELECTRICITY}"
    s = "[components.#{building_id}_#{ELECTRICITY}]\n"\
      "type = \"load\"\n"\
      "inflow = \"#{ELECTRICITY}\"\n#{scenarios_string}"
    add_if_not_added(comps, id, s)
    id
  end

  def add_building_electrical_bus(building_id, comps)
    id = "#{building_id}_electrical_bus"
    s = "[components.#{building_id}_electric_bus]\n"
    s += "type = \"muxer\"\n"
    s += "stream = \"electricity\"\n"
    s += "num_inflows = 2\n"
    s += "num_outflows = 1\n"
    s += "dispatch_strategy = \"in_order\"\n"
    add_if_not_added(comps, id, s)
    id
  end

  def add_connection(src_id, src_port, sink_id, sink_port, flow, conns)
    conns << ["#{src_id}:OUT(#{src_port})", "#{sink_id}:IN(#{sink_port})", flow]
    true
  end

  def add_electrical_source(node_id, comps)
    id = "#{node_id}_#{ELECTRICITY}_source"
    s = "[components.#{id}]\n"
    s += "type = \"source\"\n"
    s += "outflow = \"#{ELECTRICITY}\"\n"
    add_if_not_added(comps, id, s)
    id
  end

  def add_cluster_level_mux(node_id, num_inflows, num_outflows, flow, comps)
    id = "#{node_id}_#{flow}_bus"
    s = "[components.#{id}]\n"
    s += "type = \"muxer\"\n"
    s += "stream = \"#{flow}\"\n"
    s += "num_inflows = #{num_inflows}\n"
    s += "num_outflows = #{num_outflows}\n"
    s += "dispatch_strategy = \"in_order\"\n"
    add_if_not_added(comps, id, s)
    id
  end

  def _add_electrical_connections_and_components
    conns = []
    comps = {}
    node_sources = {}
    @building_config.keys.sort.each_with_index do |b_id, n|
      cfg = @building_config[b_id]
      next unless cfg[:enduses].include?(ELECTRICITY)
      eu_id = add_building_electrical_enduse(b_id, comps)
      conn_info = [eu_id, 0]
      if cfg[:has_egen]
        bus_id = add_building_electrical_bus(b_id, comps)
        cfg[:enduses] << NATURAL_GAS # to fuel the electric generator
        add_connection(bus_id, 0, eu_id, 0, ELECTRICITY, conns)
        conn_info = [bus_id, 1]
      end
      e_sup_node = cfg[:e_supply_node]
      if !e_sup_node.empty?
        if node_sources.include?(e_sup_node)
          node_sources[e_sup_node][b_id] = conn_info
        else
          node_sources[e_sup_node] = {b_id => conn_info}
        end
      end
    end
    # TODO: go through the node_configs and add any more components and connections that are electrical
    node_sources.each do |n_id, building_ids|
      n = building_ids.length
      # TODO: check the below; only want to add a source if the node is not in the @node_config map...
      src_id = add_electrical_source(n_id, comps)
      if n == 1
        b_id = building_ids.keys[0]
        sink_id, sink_port = building_ids[b_id]
        add_connection(src_id, 0, sink_id, sink_port, ELECTRICITY, conns)
      else
        mux_id = add_cluster_level_mux(n_id, 1, n, ELECTRICITY, comps)
        add_connection(src_id, 0, mux_id, 0, ELECTRICITY, conns)
        building_ids.each_with_index do |pair, idx|
          _, conn_info = pair
          sink_id, sink_port = conn_info
          add_connection(mux_id, idx, sink_id, sink_port, ELECTRICITY, conns)
        end
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
