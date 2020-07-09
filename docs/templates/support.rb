require 'set'
require 'csv'

class Support
  ELECTRICITY = 'electricity'
  NATURAL_GAS = 'natural_gas'
  HEATING = 'heating'

  attr_reader :components, :connections, :loads, :load_ids

  def self.make(load_profile, building_level, node_level)
    Support.new(
      # Load Profile
      load_profile.map {|x| x[:load_profile_scenario_id]},
      load_profile.map {|x| x[:load_profile_building_id]},
      load_profile.map {|x| x[:load_profile_enduse]},
      load_profile.map {|x| x[:load_profile_file]},
      # Building Level
      building_level.map {|x| x[:building_level_building_id]},
      building_level.map {|x| x[:building_level_egen_flag]},
      building_level.map {|x| x[:building_level_egen_eff_pct].to_f},
      building_level.map {|x| x[:building_level_heat_storage_flag]},
      building_level.map {|x| x[:building_level_heat_storage_cap_kWh].to_f},
      building_level.map {|x| x[:building_level_gas_boiler_flag]},
      building_level.map {|x| x[:building_level_gas_boiler_eff_pct].to_f},
      building_level.map {|x| x[:building_level_electricity_supply_node]},
      # Node Level
      node_level.map {|x| x[:node_level_id]},
      node_level.map {|x| x[:node_level_ng_power_plant_flag]},
      node_level.map {|x| x[:node_level_ng_power_plant_eff_pct].to_f},
      node_level.map {|x| x[:node_level_ng_supply_node]}
    )
  end

  def initialize(
    load_profile_scenario_id,
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

  def self.load_csv(csv_path)
    data = []
    headers = nil
    CSV.foreach(csv_path) do |row|
      if headers.nil?
        headers = row.map {|x| x.strip.to_sym}
      else
        data << headers.zip(row).reduce({}) do |map, hr|
          map[hr[0]] = hr[1].strip
          map
        end
      end
    end
    data
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
        egen_eff: @building_level_egen_eff_pct[n].to_f / 100.0,
        has_tes: is_true(@building_level_heat_storage_flag[n]),
        tes_cap_kWh: @building_level_heat_storage_cap_kWh[n].to_f,
        tes_max_inflow_kW: @building_level_heat_storage_cap_kWh[n].to_f / 10.0,
        has_boiler: is_true(@building_level_gas_boiler_flag[n]),
        boiler_eff: @building_level_gas_boiler_eff_pct[n].to_f / 100.0,
        e_supply_node: @building_level_electricity_supply_node[n].strip,
        ng_supply_node: "utility",
        heating_supply_node: "utility",
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
    strings.sort.join("\n")
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
      "inflow = \"#{ELECTRICITY}\"\n#{scenarios_string}\n"
    add_if_not_added(comps, id, s)
    id
  end

  def add_building_heating_enduse(building_id, comps)
    scenarios_string = make_scenario_string(building_id, HEATING)
    id = "#{building_id}_#{HEATING}"
    s = "[components.#{building_id}_#{HEATING}]\n"\
      "type = \"load\"\n"\
      "inflow = \"#{HEATING}\"\n#{scenarios_string}\n"
    add_if_not_added(comps, id, s)
    id
  end

  def add_building_electrical_bus(building_id, comps)
    id = "#{building_id}_#{ELECTRICITY}_bus"
    s = "[components.#{id}]\n"
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

  def add_natural_gas_source(node_id, comps)
    id = "#{node_id}_#{NATURAL_GAS}_source"
    s = "[components.#{id}]\n"\
      "type = \"source\"\n"\
      "outflow = \"#{NATURAL_GAS}\"\n"
    add_if_not_added(comps, id, s)
    id
  end

  def add_heating_source(node_id, comps)
    id = "#{node_id}_#{HEATING}_source"
    s = "[components.#{id}]\n"\
      "type = \"source\"\n"\
      "outflow = \"#{HEATING}\"\n"
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

  def make_building_egen_id(building_id)
    "#{building_id}_electric_generator"
  end

  def add_building_electric_generator(building_id, generator_efficiency, comps)
    id = make_building_egen_id(building_id)
    s = "[components.#{id}]\n"\
      "type = \"converter\"\n"\
      "inflow = \"natural_gas\"\n"\
      "outflow = \"electricity\"\n"\
      "lossflow = \"waste_heat\"\n"\
      "constant_efficiency = #{generator_efficiency}\n"
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
        gen_id = add_building_electric_generator(b_id, cfg[:egen_eff], comps)
        add_connection(bus_id, 0, eu_id, 0, ELECTRICITY, conns)
        add_connection(gen_id, 0, bus_id, 1, ELECTRICITY, conns)
        conn_info = [bus_id, 0]
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
        building_ids.keys.sort.each_with_index do |b_id, idx|
          sink_id, sink_port = building_ids[b_id]
          add_connection(mux_id, idx, sink_id, sink_port, ELECTRICITY, conns)
        end
      end
    end
    @connections += conns.sort
    @components += comps.keys.sort.map {|k| comps[k]}
  end

  def _add_ng_connections_and_components
    conns = []
    comps = {}
    node_sources = {}
    @building_config.keys.sort.each_with_index do |b_id, n|
      cfg = @building_config[b_id]
      if cfg[:enduses].include?(ELECTRICITY) and cfg[:has_egen] and cfg[:enduses].include?(HEATING) and cfg[:has_boiler]
        egen_id = make_building_egen_id(b_id)
        boiler_id = add_boiler(b_id, cfg[:boiler_eff], comps)
        bus_id = add_cluster_level_mux(b_id, 1, 2, NATURAL_GAS, comps)
        add_connection(bus_id, 0, egen_id, 0, NATURAL_GAS, conns)
        add_connection(bus_id, 1, boiler_id, 0, NATURAL_GAS, conns)
        conn_info = [bus_id, 0]
        ng_sup_node = cfg[:ng_supply_node]
        if !ng_sup_node.empty?
          if node_sources.include?(ng_sup_node)
            node_sources[ng_sup_node][b_id] = conn_info
          else
            node_sources[ng_sup_node] = {b_id => conn_info}
          end
        else
          raise "A building-level natural-gas electric generator is specified for "\
            "#{b_id} but no upstream natural gas supply is given..."
        end
      elsif cfg[:enduses].include?(ELECTRICITY) and cfg[:has_egen]
        egen_id = make_building_egen_id(b_id)
        conn_info = [egen_id, 0]
        ng_sup_node = cfg[:ng_supply_node]
        if !ng_sup_node.empty?
          if node_sources.include?(ng_sup_node)
            node_sources[ng_sup_node][b_id] = conn_info
          else
            node_sources[ng_sup_node] = {b_id => conn_info}
          end
        else
          raise "A building-level natural-gas electric generator is specified for "\
            "#{b_id} but no upstream natural gas supply is given..."
        end
      elsif cfg[:enduses].include?(HEATING) and cfg[:has_boiler]
        boiler_id = add_boiler(b_id, cfg[:boiler_eff], comps)
        conn_info = [boiler_id, 0]
        ng_sup_node = cfg[:ng_supply_node]
        if !ng_sup_node.empty?
          if node_sources.include?(ng_sup_node)
            node_sources[ng_sup_node][b_id] = conn_info
          else
            node_sources[ng_sup_node] = {b_id => conn_info}
          end
        else
          raise "A building-level natural-gas electric generator is specified for "\
            "#{b_id} but no upstream natural gas supply is given..."
        end
      elsif cfg[:enduses].include?(NATURAL_GAS)
        raise "NATURAL GAS enduse not implemented for #{b_id}"
      end
    end
    # TODO: go through the node_configs and add any more components and connections that are electrical
    node_sources.each do |n_id, building_ids|
      n = building_ids.length
      # TODO: check the below; only want to add a source if the node is not in the @node_config map...
      src_id = add_natural_gas_source(n_id, comps)
      if n == 1
        b_id = building_ids.keys[0]
        sink_id, sink_port = building_ids[b_id]
        add_connection(src_id, 0, sink_id, sink_port, NATURAL_GAS, conns)
      else
        mux_id = add_cluster_level_mux(n_id, 1, n, NATURAL_GAS, comps)
        add_connection(src_id, 0, mux_id, 0, NATURAL_GAS, conns)
        building_ids.each_with_index do |pair, idx|
          _, conn_info = pair
          sink_id, sink_port = conn_info
          add_connection(mux_id, idx, sink_id, sink_port, NATURAL_GAS, conns)
        end
      end
    end
    @connections += conns.sort
    @components += comps.keys.sort.map {|k| comps[k]}
  end

  def add_boiler(building_id, boiler_eff, comps)
    id = "#{building_id}_gas_boiler"
    s = "[components.#{id}]\n"\
      "type = \"converter\"\n"\
      "outflow = \"heating\"\n"\
      "inflow = \"natural_gas\"\n"\
      "lossflow = \"waste_heat\"\n"\
      "constant_efficiency = #{boiler_eff}\n"
    add_if_not_added(comps, id, s)
    id
  end

  def add_thermal_energy_storage(building_id, capacity_kWh, max_inflow_kW, comps)
    id = "#{building_id}_thermal_storage"
    s = "[components.#{id}]\n"\
      "type = \"store\"\n"\
      "outflow = \"heating\"\n"\
      "inflow = \"heating\"\n"\
      "capacity_unit = \"kWh\"\n"\
      "capacity = #{capacity_kWh}\n"\
      "max_inflow = #{max_inflow_kW}\n"
    add_if_not_added(comps, id, s)
    id
  end

  # TODO: idea
  # integrate heating, natural gas, and electrical in one "add components"
  # loop. We need to be able to do all buildng config, then all node config,
  # and THEN process all node sources
  def  _add_heating_connections_and_components
    conns = []
    comps = {}
    ht_node_sources = {}
    @building_config.keys.sort.each_with_index do |b_id, n|
      cfg = @building_config[b_id]
      next unless cfg[:enduses].include?(HEATING)
      eu_id = add_building_heating_enduse(b_id, comps)
      if cfg[:has_tes] and cfg[:has_boiler]
        tes_id = add_thermal_energy_storage(b_id, cfg[:tes_cap_kWh], cfg[:tes_max_inflow_kW], comps)
        boiler_id = add_boiler(b_id, cfg[:boiler_eff], comps)
        ht_sup_node = cfg[:heating_supply_node]
        if !ht_sup_node.empty?
          bus_id = add_cluster_level_mux(b_id, 2, 1, HEATING, comps)
          add_connection(bus_id, 0, eu_id, 0, HEATING, conns)
          add_connection(tes_id, 0, bus_id, 0, HEATING, conns)
          add_connection(boiler_id, 0, bus_id, 1, HEATING, conns)
          conn_info = [tes_id, 0]
          if ht_node_sources.include?(ht_sup_node)
            ht_node_sources[ht_sup_node][b_id] = conn_info
          else
            ht_node_sources[ht_sup_node] = {b_id => conn_info}
          end
        else
          add_connection(boiler_id, 0, tes_id, 0, HEATING, conns)
          add_connection(tes_id, 0, eu_id, 0, HEATING, conns)
        end
      elsif cfg[:has_tes]
        tes_id = add_thermal_energy_storage(b_id, cfg[:tes_cap_kWh], cfg[:tes_max_inflow_kW], comps)
        add_connection(tes_id, 0, eu_id, 0, HEATING, conns)
        conn_info = [tes_id, 0]
        ht_sup_node = cfg[:heating_supply_node]
        if !ht_sup_node.empty?
          if ht_node_sources.include?(ht_sup_node)
            ht_node_sources[ht_sup_node][b_id] = conn_info
          else
            ht_node_sources[ht_sup_node] = {b_id => conn_info}
          end
        else
          raise "A building-level TES is specified for "\
            "#{b_id} but no upstream heating supply is given..."
        end
      elsif cfg[:has_boiler]
        boiler_id = add_boiler(b_id, cfg[:boiler_eff], comps)
        ht_sup_node = cfg[:heating_supply_node]
        if !ht_sup_node.empty?
          bus_id = add_cluster_level_mux(b_id, 2, 1, HEATING, comps)
          add_connection(boiler_id, 0, bus_id, 1, HEATING, conns)
          conn_info = [bus_id, 0]
          if ht_node_sources.include?(ht_sup_node)
            ht_node_sources[ht_sup_node][b_id] = conn_info
          else
            ht_node_sources[ht_sup_node] = {b_id => conn_info}
          end
        else
          add_connection(boiler_id, 0, eu_id, 0, HEATING, conns)
        end
      else
        conn_info = [eu_id, 0]
        ht_sup_node = cfg[:heating_supply_node]
        if !ht_sup_node.empty?
          if ht_node_sources.include?(ht_sup_node)
            ht_node_sources[ht_sup_node][b_id] = conn_info
          else
            ht_node_sources[ht_sup_node] = {b_id => conn_info}
          end
        else
          raise "A building-level heating demand is specified for "\
            "#{b_id} but no upstream heating supply is given..."
        end
      end
    end
    # TODO: go through the node_configs and add any more components and connections that are electrical
    ht_node_sources.each do |n_id, building_ids|
      n = building_ids.length
      # TODO: check the below; only want to add a source if the node is not in the @node_config map...
      src_id = add_heating_source(n_id, comps)
      if n == 1
        b_id = building_ids.keys[0]
        sink_id, sink_port = building_ids[b_id]
        add_connection(src_id, 0, sink_id, sink_port, HEATING, conns)
      else
        bus_id = add_cluster_level_mux(n_id, 1, n, HEATING, comps)
        add_connection(src_id, 0, bus_id, 0, HEATING, conns)
        building_ids.each_with_index do |pair, idx|
          _, conn_info = pair
          sink_id, sink_port = conn_info
          add_connection(bus_id, idx, sink_id, sink_port, HEATING, conns)
        end
      end
    end
    @connections += conns.sort
    @components += comps.keys.sort.map {|k| comps[k]}
  end
end
