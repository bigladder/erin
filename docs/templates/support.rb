require 'set'
require 'csv'

class Support
  ELECTRICITY = 'electricity'
  NATURAL_GAS = 'natural_gas'
  HEATING = 'heating'
  COOLING = 'cooling'

  attr_reader :connections, :loads, :load_ids

  def initialize(load_profile, building_level, node_level)
    @load_profile = load_profile
    _check_load_profile_data
    @building_level = building_level
    _check_building_level_data
    @node_level = node_level
    _check_node_level_data
    _create_load_data
    _create_node_config
    _create_building_config
    @connections = []
    @comps = {}
    _add_electrical_connections_and_components
    _add_ng_connections_and_components
    _add_heating_connections_and_components
  end

  def components
    @comps.keys.sort.map {|k| @comps[k]}
  end

  # Assign an id to each component in data if it doesn't already have one
  # - data: (hash symbol any), see generate_connections for details
  # RETURN: nil, adds ids to the components if they don't already have them
  def self.ensure_components_have_ids(data)
    comp_key_and_postfixs = [
      [:source_component, [:location_id, :outflow], "source"],
      [:load_component, [:location_id, :inflow], ""],
      [:converter_component, [:location_id, :outflow], "generator"],
      [:storage_component, [:location_id, :flow], "store"],
    ]
    comp_key_and_postfixs.each do |tuple|
      key, attribs, postfix = tuple
      data.fetch(key, []).each do |c|
        next if c.include?(:id)
        parts = []
        attribs.each do |a|
          parts << c.fetch(a).to_s.strip
        end
        parts << postfix unless postfix.empty?
        c[:id] = parts.join("_")
      end
    end
  end

  # find and return the source component object for location_id and flow. If
  # not found, return nil.
  # - data: (hash symbol various), see generate_connections for details
  # - location_id: string, the location at which to find source
  # - flow: string, the source flow to find
  # RETURN: hash corresponding to the source component or nil if not found
  def self.find_flow_source_for_location(data, location_id, flow)
    src = nil
    data.fetch(:source_component, []).each do |s|
      next unless (s[:outflow] == flow) and (s[:location_id] == location_id)
      if src.nil?
        src = s
      else
        raise "Duplicate source found at location "\
          "'#{location_id}' for flow '#{flow}'"
      end
    end
    src
  end

  # Find the network links having the given flow and destination location id.
  # - nw_links: (Array {:source_location_id => string, location_id for the source of the flow,
  #                     :destination_location_id => string, location_id for the destination of the flow,
  #                     :flow => string, the type of flow (e.g., 'electricity', 'heating', etc.)})
  # RETURN: (Array string), if no link found, the array is empty. Else, filled
  # with the source ids as a strings of all sources found
  def self.find_nw_srcs_for_flow_and_dest(nw_links, flow, dest_id)
    links = []
    nw_links.each do |nw_link|
      same_flow = nw_link.fetch(:flow) == flow
      same_dest = nw_link.fetch(:destination_location_id) == dest_id
      next unless same_flow and same_dest
      links << nw_link.fetch(:source_location_id)
    end
    links
  end

  # - data: (hash symbol various), a hash table with keys
  #   - :general, (Hash symbol value)
  #   - :load_component
  #   - :load_profile, an array of Hash with keys:
  #     - :scenario_id
  #     - :building_id
  #     - :enduse
  #     - :file
  #   - :scenario, an array of Hash with keys:
  #     - :id, string, scenario id
  #     - :duration_in_hours, number, duration
  #     - :occurrence_distribution, string, id of the distribution to use
  #     - :calc_reliability, boolean string ("TRUE" or "FALSE")
  #     - :max_occurrence, integer, -1 = unlimited; otherwise, the max number
  #       of occurrences
  #   - :load_component, an array of Hash with keys:
  #     - :location_id, string, the location of the load
  #     - :inflow, string, the inflow type (e.g., 'electricity', 'heating',
  #       'cooling', 'natural_gas', etc.)
  #   - :converter_component, an array of Hash with keys:
  #     - :location_id, string, the location of the converter
  #     - :inflow, string, the inflow type (e.g., "natural_gas", "diesel", etc.)
  #     - :outflow, string, the outflow type (e.g., "electricity", "heating", etc.)
  #     - :lossflow, string, optional. defaults to "waste_heat". The lossflow stream
  #     - :constant_efficiency, number, where 0.0 < efficiency <= 1.0, the efficiency of conversion
  #   - :muxer_component, an array of Hash with keys:
  #     - :location_id, string, the location of the muxer
  #     - :flow, string, the type of flow through the muxer (e.g., 'electricity', 'natural_gas', 'coal', etc.)
  #     - :num_inflows, integer, number > 0, the number of inflow ports
  #     - :num_outflows, integer, number > 0, the number of outflow ports
  #   - :source_component, an array of Hash with keys:
  #     - :location_id, string, the location of the source
  #     - :outflow, string, the outflow provided (e.g., 'electricity', 'natural_gas', 'diesel')
  #     - :is_limited, boolean string ("TRUE" or "FALSE"), whether or not the source is limited
  #     - :max_outflow_kW, number, the maximum outflow in kW
  #   - :storage_component, an array of Hash with keys:
  #     - :location_id, string, the location of the storage
  #     - :flow, string, the flow being stored (e.g., "electricity", "diesel", etc.)
  #     - :capacity_kWh, number, > 0, the capacity of the store in kWh
  #     - :max_inflow_kW, number, >= 0, the maximum inflow to the storage in kW
  #   - :network_link, an array of Hash with keys:
  #     - :source_location_id, string, location_id for the source of the flow
  #     - :destination_location_id, string, location_id for the destination of the flow
  #     - :flow, string, the type of flow (e.g., 'electricity', 'heating', etc.)
  # for each load (and corresponding location)
  #   for each enduse
  #     find all other equipment at that location with a relevant outflow to the enduse
  #     find any inflow streams directly relevant to the enduse
  #       create muxers if more than one inflow stream is coming in
  #       record relevant connections
  #       record connection points (comp_id and port) for hookup to higher connections
  # for each location not having a load
  #   for each outflow
  #     find all equipment at that location with relevant outflow to the enduse
  #     find all outflow streams for that outflow
  #       create muxers if more than one outflow stream location
  #       pull all relevant connection points and record
  #       for all inflows, record relevant connection points for upstream 
  #       remove location from the processing list
  #
  def self.generate_connections(data)
    ensure_components_have_ids(data)
    connect_pts = {} # by node/location
    #locations = Set.new
    data[:connection] = []
    data.fetch(:load_component, []).each do |lc|
      inflow = lc.fetch(:inflow)
      loc = lc.fetch(:location_id)
      #locations << loc
      incoming_sources = find_nw_srcs_for_flow_and_dest(
        data.fetch(:network_link, []),
        inflow,
        loc)
      local_sources = []
      data.fetch(:converter_component, []).each do |cc|
        cc_loc = cc.fetch(:location_id)
        cc_outflow = cc.fetch(:outflow)
        next unless (cc_loc == loc) and (cc_outflow == inflow)
        cc_id = cc.fetch(:id)
        local_sources << cc_id
        cc_inflow = cc.fetch(:inflow)
        # Refactor the Below
        cc_source_ids = find_nw_srcs_for_flow_and_dest(
          data.fetch(:network_link, []),
          cc_inflow,
          loc)
        cc_source_ids.each do |src_id|
          if connect_pts.include?(src_id)
            connect_pts[src_id][cc_inflow] = [cc_id, 0]
          else
            connect_pts[src_id] = {cc_inflow => [cc_id, 0]}
          end
        end
      end
      local_stores = []
      data.fetch(:storage_component, []).each do |store|
        store_loc = store.fetch(:location_id)
        store_flow = store.fetch(:flow)
        next unless (store_loc == loc) and (store_flow == inflow)
        store_id = store.fetch(:id)
        local_stores << store_id
        store_flow = store.fetch(:flow)
        # Refactor the Below
        store_source_ids = find_nw_srcs_for_flow_and_dest(
          data.fetch(:network_link, []),
          store_flow,
          loc)
        store_source_ids.each do |src_id|
          if connect_pts.include?(src_id)
            connect_pts[src_id][store_flow] = [store_id, 0]
          else
            connect_pts[src_id] = {store_flow => [store_id, 0]}
          end
        end
      end
      num_incoming = incoming_sources.length
      num_local = local_sources.length
      num_stores = local_stores.length
      num_sources = num_incoming + num_local
      if num_sources == 1 and num_incoming == 1 and num_stores == 1
        src = incoming_sources[0]
        store_id = local_stores[0]
        if connect_pts.include?(src)
          # NOTE: change connect_pts to: (Map SourceLocationID (Map FlowID (Array (Tuple CompID InflowPort))))
          connect_pts[src][inflow] = [store_id, 0]
        else
          connect_pts[src] = {inflow => [store_id, 0]}
        end
        data[:connection] << [
          "#{store_id}:OUT(0)",
          "#{lc.fetch(:id)}:IN(0)",
          inflow,
        ]
      elsif num_sources == 1 and num_incoming == 1 and num_stores == 0
        src = incoming_sources[0]
        if connect_pts.include?(src)
          # NOTE: change connect_pts to: (Map SourceLocationID (Map FlowID (Array (Tuple CompID InflowPort))))
          connect_pts[src][inflow] = [lc.fetch(:id), 0]
        else
          connect_pts[src] = {inflow => [lc.fetch(:id), 0]}
        end
      elsif num_sources == 1 and num_local == 1 and num_stores == 0
        raise "not implemented"
        # ...
      elsif num_sources > 1
        new_mux = {
          location_id: loc,
          id: "#{loc}_#{inflow}_bus",
          flow: inflow,
          num_inflows: num_sources,
          num_outflows: 1,
        }
        if data.include?(:muxer_component)
          data[:muxer_component] << new_mux
        else
          data[:muxer_component] = [new_mux]
        end
        if num_stores == 0
          data[:connection] << [
            "#{new_mux.fetch(:id)}:OUT(0)",
            "#{lc.fetch(:id)}:IN(0)",
            inflow,
          ]
        elsif num_stores == 1
          store = local_stores[0]
          data[:connection] << [
            "#{store.fetch(:id)}:OUT(0)",
            "#{lc.fetch(:id)}:IN(0)",
            inflow,
          ]
        elsif num_stores > 1
        end
        local_sources.each_with_index do |src_id, idx|
          port = idx + num_incoming
          data[:connection] << [
            "#{src_id}:OUT(0)",
            "#{new_mux.fetch(:id)}:IN(#{port})",
            inflow]
        end
        if num_incoming == 1
          src = incoming_sources[0]
          if connect_pts.include?(src)
            connect_pts[src][inflow] = [new_mux.fetch(:id), 0]
          else
            connect_pts[src] = {inflow => [new_mux.fetch(:id), 0]}
          end
        else
          raise "not implemented"
        end
      end
      #srcs = data[:source_component].select {|c| c[:location_id] == loc and c[:outflow] == inflow}
      #num = srcs.length
      ##convs = data[:converter_components].select {|c| c[:location_id] == loc and c[:outflow] == inflow}
      ##store = 
      #incoming_sources = []
      #if num == 0
      #elsif num == 1
      #else
      #end
    end
    connect_pts.each do |loc, flow_map|
      flow_map.each do |flow, conn_info|
        src = find_flow_source_for_location(data, loc, flow)
        next if src.nil?
        id, port = conn_info
        data[:connection] << [src.fetch(:id) + ":OUT(0)", "#{id}:IN(#{port})", flow]
      end
    end
    # for each load (and corresponding location)
    #   for each enduse
    #     find all other equipment at that location with a relevant outflow to the enduse
    #     find any inflow streams directly relevant to the enduse
    #       create muxers if more than one inflow stream is coming in
    #       record relevant connections
    #       record connection points (comp_id and port) for hookup to higher connections
    # for each location not having a load
    #   for each outflow
    #     find all equipment at that location with relevant outflow to the enduse
    #     find all outflow streams for that outflow
    #       create muxers if more than one outflow stream location
    #       pull all relevant connection points and record
    #       for all inflows, record relevant connection points for upstream 
    #       remove location from the processing list
    data
  end

  # - csv_path: string, the path to a CSV file
  # ASSUMPTION: The file has headers in the first row and each row is the same
  # length.
  # RETURN: (array (hash symbol string)), the data read in
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

  # - csv_path: string, the path to a CSV file
  # ASSUMPTION: The file has headers in the first row and only one row of data
  # (on the second row). Rows are the same length.
  # RETURN: (hash symbol string), the data read in
  def self.load_key_value_csv(csv_path)
    data = {}
    headers = nil
    CSV.foreach(csv_path) do |row|
      if headers.nil?
        headers = row.map {|x| x.strip.to_sym}
        headers.each do |h|
          data[h] = nil
        end
      else
        headers.zip(row).each do |hr|
          data[hr[0]] = hr[1].strip
        end
        break
      end
    end
    data
  end

  private

  def _create_load_data
    @loads = []
    @load_ids = []
    load_id_set = Set.new
    @load_profile.each do |item|
      b_id = item[:building_id]
      s_id = item[:scenario_id]
      enduse = item[:enduse]
      file = item[:file]
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
        puts "array_of_arrays:\n#{array_of_arrays}"
        puts "array_of_arrays.nil? = #{array_of_arrays.nil?}"
        puts "array_of_arrays.length = #{array_of_arrays.length}"
        raise "Array lengths not the same #{msg}"
      end
    end
  end

  def _assert_string_not_empty(map, key)
    x = map[key]
    msg = "ERROR! Value for key #{key} cannot be empty in #{map}"
    raise msg if x.to_s.strip.empty?
  end

  def _assert_proper_enduse(x, msg)
    raise msg unless [ELECTRICITY, HEATING, COOLING].include?(x)
  end

  def _assert_proper_flag(map, key)
    x = map[key]
    dc_x = x.to_s.strip.downcase
    if x == true or x == false or dc_x == "true" or dc_x == "false"
      return
    end
    raise "ERROR! Value at key #{key} is not a proper flag in #{map}"
  end

  # INTERNAL METHOD
  def _check_load_profile_data
    @load_profile.each do |item|
      _assert_string_not_empty(item, :scenario_id)
      _assert_string_not_empty(item, :building_id)
      _assert_proper_enduse(
        item[:enduse],
        "ERROR! unexpected enduse for #{item}")
      _assert_string_not_empty(item, :file)
    end
  end

  def _assert_float_within_range(map, key, low, high)
    f = map[key].to_f
    if (f >= low) and (f <= high)
      return
    end
    raise "ERROR! Value at key `#{key}` (#{f}) is not between "\
      "#{low} and #{high} (inclusive) in #{map}"
  end

  def _assert_positive_number(map, key)
    x = map[key].to_f
    return if x > 0.0
    raise "ERROR! Value at key `#{key}` not greater than 0.0 in #{map}"
  end

  # INTERNAL METHOD
  def _check_building_level_data
    @building_level.each do |item|
      _assert_string_not_empty(item, :id)
      _assert_proper_flag(item, :egen_flag)
      if is_true(item[:egen_flag])
        _assert_float_within_range(item, :egen_eff_pct, 0.01, 100.0)
      end
      _assert_proper_flag(item, :heat_storage_flag)
      if is_true(item[:heat_storage_flag])
        _assert_positive_number(item, :heat_storage_cap_kWh)
      end
      _assert_proper_flag(item, :gas_boiler_flag)
      if is_true(item[:gas_boiler_flag])
        _assert_float_within_range(item, :gas_boiler_eff_pct, 0.01, 100.0)
      end
    end
  end

  # INTERNAL METHOD
  def _check_node_level_data
    @node_level.each do |item|
      _assert_string_not_empty(item, :id)
      _assert_proper_flag(item, :ng_power_plant_flag)
      if is_true(item[:ng_power_plant_flag])
        _assert_float_within_range(item, :ng_power_plant_eff_pct, 0.01, 100.0)
      end
    end
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
    @load_profile.each do |item|
      next unless item[:building_id] == id
      enduses << item[:enduse]
    end
    enduses
  end

  # INTERNAL METHOD
  def _create_building_config
    return if !@building_config.nil?
    @building_config = {}
    @building_level.each do |item|
      b_id = item[:id]
      if @building_config.include?(b_id)
        raise "Building ID \"#{b_id}\" is multiply defined!"
      end
      @building_config[b_id] = {
        has_egen: is_true(item[:egen_flag]),
        egen_eff: item[:egen_eff_pct].to_f / 100.0,
        has_tes: is_true(item[:heat_storage_flag]),
        tes_cap_kWh: item[:heat_storage_cap_kWh].to_f,
        tes_max_inflow_kW: item[:heat_storage_cap_kWh].to_f / 10.0,
        has_boiler: is_true(item[:gas_boiler_flag]),
        boiler_eff: item[:gas_boiler_eff_pct].to_f / 100.0,
        e_supply_node: item[:electricity_supply_node].strip,
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
    @node_level.each do |item|
      n_id = item[:id]
      if @node_config.include?(n_id)
        raise "node id \"#{n_id}\" multiply defined in node_config"
      end
      @node_config[n_id] = {
        has_ng_pwr_plant: is_true(item[:ng_power_plant_flag]),
        ng_pwr_plant_eff: item[:ng_power_plant_eff_pct].to_f / 100.0,
        ng_supply_node: item[:ng_supply_node].strip,
      }
    end
  end

  def make_scenario_string(building_id, enduse)
    strings = []
    @building_level.each do |item|
      b_id = item[:id]
      next unless building_id == b_id
      @load_profile.each_with_index do |lp_item, idx|
        s_id = lp_item[:scenario_id]
        next unless lp_item[:building_id] == b_id
        next unless lp_item[:enduse] == enduse
        load_id = @load_ids[idx]
        strings << "loads_by_scenario.#{ s_id } = \"#{ load_id }\""
      end
    end
    strings.sort.join("\n")
  end

  def add_if_not_added(id, string)
    if !@comps.include?(id)
      value = {id: id, string: string}
      @comps[id] = value
      true
    else
      false
    end
  end

  def add_converter(id, const_eff, inflow, outflow, lossflow = nil)
    lossflow = "waste_heat" if lossflow.nil?
    s = "[components.#{id}]\n"\
      "type = \"converter\"\n"\
      "inflow = \"#{inflow}\"\n"\
      "outflow = \"#{outflow}\"\n"\
      "lossflow = \"#{lossflow}\"\n"\
      "constant_efficiency = #{const_eff}\n"
    add_if_not_added(id, s)
    id
  end

  def add_load(building_id, enduse)
    scenarios_string = make_scenario_string(building_id, enduse)
    id = "#{building_id}_#{enduse}"
    s = "[components.#{building_id}_#{enduse}]\n"\
      "type = \"load\"\n"\
      "inflow = \"#{enduse}\"\n#{scenarios_string}\n"
    add_if_not_added(id, s)
    id
  end

  def add_building_electrical_enduse(building_id)
    add_load(building_id, ELECTRICITY)
  end

  def add_building_heating_enduse(building_id)
    add_load(building_id, HEATING)
  end

  def add_muxer(id, stream, num_inflows, num_outflows)
    s = "[components.#{id}]\n"\
      "type = \"muxer\"\n"\
      "stream = \"#{stream}\"\n"\
      "num_inflows = #{num_inflows}\n"\
      "num_outflows = #{num_outflows}\n"\
      "dispatch_strategy = \"in_order\"\n"
    add_if_not_added(id, s)
    id
  end

  def add_building_electrical_bus(building_id)
    id = "#{building_id}_#{ELECTRICITY}_bus"
    add_muxer(id, ELECTRICITY, 2, 1)
  end

  def add_connection(src_id, src_port, sink_id, sink_port, flow, conns)
    conns << ["#{src_id}:OUT(#{src_port})", "#{sink_id}:IN(#{sink_port})", flow]
    true
  end

  def add_source(id, outflow)
    s = "[components.#{id}]\n"\
      "type = \"source\"\n"\
      "outflow = \"#{outflow}\"\n"
    add_if_not_added(id, s)
    id
  end

  def add_electrical_source(node_id)
    id = "#{node_id}_#{ELECTRICITY}_source"
    add_source(id, ELECTRICITY)
  end

  def add_natural_gas_source(node_id)
    id = "#{node_id}_#{NATURAL_GAS}_source"
    add_source(id, NATURAL_GAS)
  end

  def add_heating_source(node_id)
    id = "#{node_id}_#{HEATING}_source"
    add_source(id, HEATING)
  end

  def add_cluster_level_mux(node_id, num_inflows, num_outflows, flow)
    id = "#{node_id}_#{flow}_bus"
    add_muxer(id, flow, num_inflows, num_outflows)
  end

  def make_building_egen_id(building_id)
    "#{building_id}_electric_generator"
  end

  def add_building_electric_generator(building_id, generator_efficiency)
    id = make_building_egen_id(building_id)
    add_converter(id, generator_efficiency, NATURAL_GAS, ELECTRICITY)
  end

  def _add_electrical_connections_and_components
    conns = []
    node_sources = {}
    @building_config.keys.sort.each_with_index do |b_id, n|
      cfg = @building_config[b_id]
      next unless cfg[:enduses].include?(ELECTRICITY)
      eu_id = add_building_electrical_enduse(b_id)
      conn_info = [eu_id, 0]
      if cfg[:has_egen]
        bus_id = add_building_electrical_bus(b_id)
        gen_id = add_building_electric_generator(b_id, cfg[:egen_eff])
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
    @node_config.keys.sort.each_with_index do |n_id, idx|
      cfg = @node_config[n_id]
      if cfg[:has_ng_pwr_plant] and node_sources.include?(n_id)
        pp_id = add_converter(
          "#{n_id}_ng_power_plant",
          cfg[:ng_pwr_plant_eff],
          NATURAL_GAS, ELECTRICITY)
        n = node_sources[n_id].length
        if n == 1
          b_id = node_sources[n_id].keys[0]
          tgt_id, tgt_port = node_sources[n_id][b_id]
          add_connection(pp_id, 0, tgt_id, tgt_port, ELECTRICITY, conns)
        else
          bus_id = add_cluster_level_mux(n_id, 1, n, ELECTRICITY)
          add_connection(pp_id, 0, bus_id, 0, ELECTRICITY, conns)
          node_sources[n_id].keys.sort.each_with_index do |b_id, idx|
            tgt_id, tgt_port = node_sources[n_id][b_id]
            add_connection(bus_id, idx, tgt_id, tgt_port, ELECTRICITY, conns)
          end
        end
      end
    end
    node_sources.each do |n_id, building_ids|
      n = building_ids.length
      next if @node_config.include?(n_id)
      src_id = add_electrical_source(n_id)
      if n == 1
        b_id = building_ids.keys[0]
        sink_id, sink_port = building_ids[b_id]
        add_connection(src_id, 0, sink_id, sink_port, ELECTRICITY, conns)
      else
        mux_id = add_cluster_level_mux(n_id, 1, n, ELECTRICITY)
        add_connection(src_id, 0, mux_id, 0, ELECTRICITY, conns)
        building_ids.keys.sort.each_with_index do |b_id, idx|
          sink_id, sink_port = building_ids[b_id]
          add_connection(mux_id, idx, sink_id, sink_port, ELECTRICITY, conns)
        end
      end
    end
    @connections += conns.sort
  end

  def _add_ng_connections_and_components
    conns = []
    node_sources = {}
    @building_config.keys.sort.each_with_index do |b_id, n|
      cfg = @building_config[b_id]
      if cfg[:enduses].include?(ELECTRICITY) and cfg[:has_egen] and cfg[:enduses].include?(HEATING) and cfg[:has_boiler]
        egen_id = make_building_egen_id(b_id)
        boiler_id = add_boiler(b_id, cfg[:boiler_eff])
        bus_id = add_cluster_level_mux(b_id, 1, 2, NATURAL_GAS)
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
        boiler_id = add_boiler(b_id, cfg[:boiler_eff])
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
    @node_config.keys.sort.each_with_index do |n_id, idx|
      cfg = @node_config[n_id]
      if cfg[:has_ng_pwr_plant]
        pp_id = add_converter(
          "#{n_id}_ng_power_plant",
          cfg[:ng_pwr_plant_eff],
          NATURAL_GAS, ELECTRICITY)
        ng_sup_node = cfg[:ng_supply_node]
        conn_info = [pp_id, 0]
        if !ng_sup_node.empty?
          if node_sources.include?(ng_sup_node)
            node_sources[ng_sup_node][n_id] = conn_info
          else
            node_sources[ng_sup_node] = {n_id => conn_info}
          end
        else
          raise "#{NATURAL_GAS} supply node empty but a #{NATURAL_GAS} "\
            "power-plant requires power for node #{n_id}"
        end
      end
    end
    node_sources.each do |n_id, building_ids|
      n = building_ids.length
      next if @node_config.include?(n_id)
      src_id = add_natural_gas_source(n_id)
      if n == 1
        b_id = building_ids.keys[0]
        sink_id, sink_port = building_ids[b_id]
        add_connection(src_id, 0, sink_id, sink_port, NATURAL_GAS, conns)
      else
        mux_id = add_cluster_level_mux(n_id, 1, n, NATURAL_GAS)
        add_connection(src_id, 0, mux_id, 0, NATURAL_GAS, conns)
        building_ids.each_with_index do |pair, idx|
          _, conn_info = pair
          sink_id, sink_port = conn_info
          add_connection(mux_id, idx, sink_id, sink_port, NATURAL_GAS, conns)
        end
      end
    end
    @connections += conns.sort
  end

  def add_boiler(building_id, boiler_eff)
    id = "#{building_id}_gas_boiler"
    add_converter(id, boiler_eff, NATURAL_GAS, HEATING)
  end

  def add_store(id, stream, capacity_kWh, max_inflow_kW)
    s = "[components.#{id}]\n"\
      "type = \"store\"\n"\
      "outflow = \"#{stream}\"\n"\
      "inflow = \"#{stream}\"\n"\
      "capacity_unit = \"kWh\"\n"\
      "capacity = #{capacity_kWh}\n"\
      "max_inflow = #{max_inflow_kW}\n"
    add_if_not_added(id, s)
    id
  end

  def add_thermal_energy_storage(building_id, capacity_kWh, max_inflow_kW)
    id = "#{building_id}_thermal_storage"
    add_store(id, HEATING, capacity_kWh, max_inflow_kW)
  end

  # TODO: idea
  # integrate heating, natural gas, and electrical in one "add components"
  # loop. We need to be able to do all buildng config, then all node config,
  # and THEN process all node sources
  def  _add_heating_connections_and_components
    conns = []
    ht_node_sources = {}
    @building_config.keys.sort.each_with_index do |b_id, n|
      cfg = @building_config[b_id]
      next unless cfg[:enduses].include?(HEATING)
      eu_id = add_building_heating_enduse(b_id)
      if cfg[:has_tes] and cfg[:has_boiler]
        tes_id = add_thermal_energy_storage(b_id, cfg[:tes_cap_kWh], cfg[:tes_max_inflow_kW])
        boiler_id = add_boiler(b_id, cfg[:boiler_eff])
        ht_sup_node = cfg[:heating_supply_node]
        if !ht_sup_node.empty?
          bus_id = add_cluster_level_mux(b_id, 2, 1, HEATING)
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
        tes_id = add_thermal_energy_storage(b_id, cfg[:tes_cap_kWh], cfg[:tes_max_inflow_kW])
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
        boiler_id = add_boiler(b_id, cfg[:boiler_eff])
        ht_sup_node = cfg[:heating_supply_node]
        if !ht_sup_node.empty?
          bus_id = add_cluster_level_mux(b_id, 2, 1, HEATING)
          add_connection(boiler_id, 0, bus_id, 1, HEATING, conns)
          add_connection(bus_id, 0, eu_id, 0, HEATING, conns)
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
      src_id = add_heating_source(n_id)
      if n == 1
        b_id = building_ids.keys[0]
        sink_id, sink_port = building_ids[b_id]
        add_connection(src_id, 0, sink_id, sink_port, HEATING, conns)
      else
        bus_id = add_cluster_level_mux(n_id, 1, n, HEATING)
        add_connection(src_id, 0, bus_id, 0, HEATING, conns)
        building_ids.each_with_index do |pair, idx|
          _, conn_info = pair
          sink_id, sink_port = conn_info
          add_connection(bus_id, idx, sink_id, sink_port, HEATING, conns)
        end
      end
    end
    @connections += conns.sort
  end
end
