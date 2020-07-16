require 'set'
require 'csv'

class Support
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
  #   - :pass_through_component, an array of Hash with keys:
  #     - :location_id, string, the location of the pass-through
  #     - :flow, string, the flow being passed through (e.g., 'electricity', 'diesel')
  #   - :network_link, an array of Hash with keys:
  #     - :source_location_id, string, location_id for the source of the flow
  #     - :destination_location_id, string, location_id for the destination of the flow
  #     - :flow, string, the type of flow (e.g., 'electricity', 'heating', etc.)

  attr_reader(
    :connections,
    :converter_component,
    :load_component,
    :muxer_component,
    :network_link,
    :source_component,
    :storage_component,
  )

  def initialize(data)
    @ids_in_use = Set.new
    @converter_component = data.fetch(:converter_component, [])
    @load_component = data.fetch(:load_component, [])
    @muxer_component = data.fetch(:muxer_component, [])
    @network_link = data.fetch(:network_link, [])
    @source_component = data.fetch(:source_component, [])
    @storage_component = data.fetch(:storage_component, [])
    ensure_components_have_ids
    @connections = []
    generate_connections
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

  # - id: string, the identity to make unique
  # SIDE-EFFECT: stores the unique identity in "ids_in_use" to ensure we don't
  # duplicate an id. Makes an id unique by adding a number at the end of the
  # standard tag or incrementing an existing number
  # RETURN: string, the unique identity
  def make_id_unique(id)
    test_id = id
    while @ids_in_use.include?(test_id)
      m = test_id.match(/(.*?)(\d*)$/)
      n = if m[2].empty? then 1 else m[2].to_i + 1 end
      if m[1].end_with?("_")
        test_id = m[1] + n.to_s
      else
        test_id = m[1] + "_" + n.to_s
      end
    end
    @ids_in_use << test_id
    test_id
  end

  # Assign an id to each component if it doesn't already have one
  # RETURN: nil, adds ids to the components if they don't already have them
  def ensure_components_have_ids
    comp_infos = [
      [@source_component, [:location_id, :outflow], "source"],
      [@load_component, [:location_id, :inflow], ""],
      [@converter_component, [:location_id, :outflow], "generator"],
      [@storage_component, [:location_id, :flow], "store"],
    ]
    comp_infos.each do |tuple|
      cs, attribs, postfix = tuple
      cs.each do |c|
        next if c.include?(:id)
        parts = []
        attribs.each do |a|
          parts << c.fetch(a).to_s.strip
        end
        parts << postfix unless postfix.empty?
        id = make_id_unique(parts.join("_"))
        c[:id] = id
      end
    end
    nil
  end

  # - loc: string, location id
  # - flow: string, flow id
  # - comp: (Array Hash), the array of components
  # - loc_key: symbol, the key into the items in the array of components above
  #   for location to filter on
  # - flow_key: symbol, the key into the items in the array of components that
  #   yields flow to filter on
  # - container: (or nil container), an optional container to pass in and use
  # RETURN: (Container Hash), the components selected by location and flow
  # Note: defaults to a container type of Array
  def get_components_by_location_and_flow(loc, flow, comp, loc_key, flow_key, container=nil)
    cs = container || []
    comp.each do |c|
      same_flow = c.fetch(flow_key) == flow
      same_loc = c.fetch(loc_key) == loc
      next unless same_flow and same_loc
      cs << c
    end
    cs
  end

  def get_conn_info_by_location_and_flow(loc, flow, comp, loc_key, flow_key, ports)
    get_components_by_location_and_flow(
      loc, flow, comp, loc_key, flow_key).map {|c| [c.fetch(:id)] + ports}
  end

  # - location_id: string, the id for the location to add the mux
  # - flow: string, id for the flow to use
  # - num_inflows: integer, num_inflows > 0, the number of inflows
  # - num_outflows: integer, num_outflows > 0, the number of outflows
  # SIDE_EFFECTS: adds the new mux to @muxer_component
  # RETURN: string, id of the new mux added
  def add_muxer_component(location_id, flow, num_inflows, num_outflows)
    id = make_id_unique("#{location_id}_#{flow}_bus")
    new_mux = {
      location_id: location_id,
      id: id,
      flow: flow,
      num_inflows: num_inflows,
      num_outflows: num_outflows,
    }
    @muxer_component << new_mux
    id
  end

  # - src_id: string, the source id
  # - src_port: integer, the source port
  # - sink_id: string, the sink id
  # - sink_port: integer, the sink port
  # - flow: string, the flow
  # SIDE-EFFECT: adds a new connection to the @connections array
  # RETURN: nil
  def add_connection(src_id, src_port, sink_id, sink_port, flow)
    @connections << [
      "#{src_id}:OUT(#{src_port})",
      "#{sink_id}:IN(#{sink_port})",
      flow,
    ]
    nil
  end

  # RETURN: (Set string), all locations referenced by all components
  def get_all_locations
    comps = [
      @source_component,
      @load_component,
      @converter_component,
      @storage_component,
    ]
    locations = Set.new
    comps.each do |cs|
      cs.each do |c|
        locations << c[:location_id]
      end
    end
    locations
  end

  # - loc: string, the location id
  # RETURN: (Array string), the flows at a given location
  def flows_at_location(loc)
    flows = Set.new
    comps = [
      [@load_component, [:location_id], [:inflow]],
      [@converter_component, [:location_id], [:inflow, :outflow]],
      [@source_component, [:location_id], [:outflow]],
      [@storage_component, [:location_id], [:flow]],
      [@network_link, [:source_location_id, :destination_location_id], [:flow]],
    ]
    comps.each do |cs, loc_keys, flow_keys|
      cs.each do |c|
        flag = loc_keys.reduce(false) {|f, k| f or (c.fetch(k)==loc)}
        next unless flag
        flow_keys.each {|k| flows << c.fetch(k)}
      end
    end
    flows.to_a.sort
  end

  def get_outbound_links(source_location_id, flow_id)
    get_components_by_location_and_flow(
      source_location_id, flow_id,
      @network_link, :source_location_id, :flow)
  end

  def get_inbound_links(dest_location_id, flow_id)
    get_components_by_location_and_flow(
      dest_location_id, flow_id,
      @network_link, :destination_location_id, :flow)
  end

  def get_internal_loads(loc, flow)
    inflow_port = 0
    get_conn_info_by_location_and_flow(
      loc, flow,
      @converter_component, :location_id, :inflow, [inflow_port])
  end

  def get_loads(loc, flow)
    inflow_port = 0
    get_conn_info_by_location_and_flow(
      loc, flow,
      @load_component, :location_id, :inflow, [inflow_port])
  end

  def get_stores(loc, flow)
    inflow_port = 0
    outflow_port = 0
    get_conn_info_by_location_and_flow(
      loc, flow,
      @storage_component, :location_id, :flow, [inflow_port, outflow_port])
  end

  def get_sources(loc, flow)
    outflow_port = 0
    get_conn_info_by_location_and_flow(
      loc, flow,
      @source_component, :location_id, :outflow, [outflow_port])
  end

  def get_converters(loc, outflow)
    outflow_port = 0
    get_conn_info_by_location_and_flow(
      loc, outflow,
      @converter_component, :location_id, :outflow, [outflow_port])
  end

  def assign_in_hash(h, keys, value)
    h_ = h
    last_idx = keys.length - 1
    keys.each_with_index do |k, idx|
      if idx == last_idx
        h_[k] = value
      else
        if !h_.include?(k)
          h_[k] = {}
        end
        h_ = h_[k]
      end
    end
    h
  end

  def update_in_hash(h, keys)
    h_ = h
    last_idx = keys.length - 1
    keys.each_with_index do |k, idx|
      if idx == last_idx
        h_[k] = yield(h_[k])
      else
        if !h_.include?(k)
          h_[k] = {}
        end
        h_ = h_[k]
      end
    end
    h
  end

  # - h: hash, the hash to update. Should contain count_key and conn_key
  # - ref_conn: hash, reference for connection info by loc and flow
  # - loc: string, location id
  # - flow: string, flow_id
  # - is_source: bool, true if source, false if for dest
  # - count_key: symbol, the key to query from h to get the count
  # - conn_key: symbol, key to add connections to in h
  # RETURN: nil
  # SIDE-EFFECTS: Adds an array of all connections for linking to the conn_key of h
  def add_link_connect_points(h, ref_conn, loc, flow, is_source)
    count_key, conn_key = (
      is_source ? [:source_count, :source] : [:dest_count, :dest])
    tgt = ref_conn.fetch(loc, {}).fetch(flow, nil)
    count = h[count_key]
    if !tgt.nil?
      if count == 1
        h[conn_key] = [tgt]
      elsif count > 1
        bus = nil
        if is_source
          bus = add_muxer_component(loc, flow, 1, count)
          add_connection(tgt[0], tgt[1], bus, 0, flow)
        else
          bus = add_muxer_component(loc, flow, count, 1)
          add_connection(bus, 0, tgt[0], tgt[1], flow)
        end
        h[conn_key] = []
        count.times {|idx| h[conn_key] << [bus, idx]}
      end
    end
  end

  def connect_sinks_to_upstream_bus(groups, bus, flow, max=nil, start_port=nil)
    n = 0
    conn_port = start_port || 0
    groups.each do |conns|
      conns.each_with_index do |conn, idx|
        if !max.nil? and n >= max
          puts "groups:\n#{groups}"
          puts "bus:\n#{bus}"
          puts "flow:\n#{flow}"
          puts "max:\n#{max}"
          puts "start_port:\n#{start_port}"
          puts "n:\n#{n}"
          raise "More than the maximum number of connections made!"
        end
        id, port = conn
        add_connection(bus, conn_port, id, port, flow)
        conn_port += 1
        n += 1
      end
    end
    n
  end

  def connect_sources_to_downstream_bus(groups, bus, flow, max=nil, start_port=nil)
    n = 0
    conn_port = start_port || 0
    groups.each do |conns|
      conns.each_with_index do |conn, idx|
        if !max.nil? and n >= max
          raise "More than the maximum number of connections made!"
        end
        id, port = conn
        add_connection(id, port, bus, conn_port, flow)
        conn_port += 1
        n += 1
      end
    end
    n
  end

  def generate_connections
    inflow_points = {} # (Hash LocationID (Hash FuelID (Tuple ComponentID Port)))
    outflow_points = {} # (Hash LocationID (Hash FuelID (Tuple ComponentID Port)))
    locations = get_all_locations
    locations.each do |loc|
      flows_at_location(loc).each do |flow|
        outbound_links = get_outbound_links(loc, flow)
        inbound_links = get_inbound_links(loc, flow)
        internal_loads = get_internal_loads(loc, flow)
        loads = get_loads(loc, flow)
        stores = get_stores(loc, flow)
        sources = get_sources(loc, flow)
        converters = get_converters(loc, flow)
        num_outbound = outbound_links.length
        num_inbound = inbound_links.length
        num_internal_loads = internal_loads.length
        num_loads = loads.length
        num_stores = stores.length
        num_sources = sources.length
        num_converters = converters.length
        # FINAL OUTFLOWS / STORAGE COMPONENTS INTERFACE
        num_total_outflows = (num_outbound > 0 ? 1 : 0) + num_internal_loads + num_loads
        num_total_inflows = num_converters + (num_inbound > 0 ? 1 : 0) + num_sources    
        if num_total_outflows == 1 and num_stores == 1
          store_id, _, store_outport = stores[0]
          connect_sinks_to_upstream_bus(
            [loads, internal_loads], store_id, flow, 1)
          if num_outbound > 0
            conn_info = [store_id, store_outport]
            assign_in_hash(outflow_points, [loc, flow], conn_info)
          end
        elsif num_total_outflows > 0 and num_stores > 0
          bus = add_muxer_component(
            loc, flow, num_total_inflows, num_total_outflows)
          connect_sinks_to_upstream_bus(
            [loads, internal_loads], bus, flow)
          if num_outbound > 0
            conn_info = [bus, num_loads + num_internal_loads]
            assign_in_hash(outflow_points, [loc, flow], conn_info)
          end
          connect_sources_to_downstream_bus([stores], bus, flow)
        end
        # STORAGE COMPONENTS AND FINAL OUTPUTS / CONVERTERS, INFLOWS, and SOURCES INTERFACE
        if num_total_inflows == 1 and num_stores == 1
          store_id, store_port, _ = stores[0]
          next_port = connect_sources_to_downstream_bus(
            [sources], store_id, flow, 1)
          if num_inbound > 0
            conn_info = [store_id, store_port]
            assign_in_hash(inflow_points, [loc, flow], conn_info)
          end
          connect_sources_to_downstream_bus(
            [converters], store_id, flow, 1, next_port) if next_port == 0 and num_inbound == 0
        elsif num_total_inflows > 0 and num_stores > 0
          bus = add_muxer_component(
            loc, flow, num_total_inflows, num_stores)
          connect_sinks_to_upstream_bus([stores], bus, flow)
          connect_sources_to_downstream_bus([sources], bus, flow)
          assign_in_hash(inflow_points, [loc, flow], [bus, num_sources]) if num_inbound > 0
          connect_sources_to_downstream_bus(
            [converters], bus, flow, nil,
            num_sources + (num_inbound > 0 ? 1 : 0))
        elsif num_stores == 0 and num_total_outflows == 1 and num_total_inflows == 1
          sources.each do |sc|
            sc_id, sc_port = sc
            if num_loads > 0 or num_internal_loads > 0
              connect_sinks_to_upstream_bus(
                [loads, internal_loads], sc_id, flow, 1, sc_port)
            end
            assign_in_hash(outflow_points, [loc, flow], sc) if num_outbound > 0
          end
          if num_inbound > 0
            loads.each do |ld|
              assign_in_hash(inflow_points, [loc, flow], ld)
            end
            internal_loads.each do |il|
              assign_in_hash(inflow_points, [loc, flow], il)
            end
            if num_outbound > 0
              # for locations A, B, C, we have a link from A->B and B->C with
              # some fuel but nothing is going on at B. Should we delete A->B
              # and B->C and add A->C?
              raise "not implemented"
            end
          end
          converters.each do |cc|
            id, port = cc
            if num_loads > 0 or num_internal_loads > 0
              connect_sinks_to_upstream_bus(
                [loads, internal_loads], id, flow, 1, port)
            end
            assign_in_hash(outflow_points, [loc, flow], cc) if num_outbound > 0
          end
        elsif num_stores == 0 and num_total_outflows > 0 and num_total_inflows > 0
          inbus = add_muxer_component(
            loc, flow,
            num_total_inflows,
            (num_stores > 0 ? num_stores : num_total_outflows)
          )
          connect_sources_to_downstream_bus(
            [sources], inbus, flow)
          if num_inbound > 0
            conn_info = [inbus, num_sources]
            assign_in_hash(inflow_points, [loc, flow], conn_info)
          end
          connect_sources_to_downstream_bus(
            [converters], inbus, flow,
            nil, num_sources + (num_inbound > 0 ? 1 : 0))
          if num_stores > 0
            connect_sinks_to_upstream_bus(
              [stores], inbus, flow)
          elsif num_total_outflows > 0
            connect_sinks_to_upstream_bus(
              [loads, internal_loads], inbus, flow)
            if num_outbound > 0
              conn_info = [inbus, num_loads + num_internal_loads]
              assign_in_hash(outflow_points, [loc, flow], conn_info)
            end
          end
        end
      end
    end
    points = {}
    @network_link.each do |link|
      src = link.fetch(:source_location_id)
      dest = link.fetch(:destination_location_id)
      flow = link.fetch(:flow)
      update_in_hash(
        points, [src, flow, :source_count]) {|v| (v.nil? ? 1 : v + 1)}
      update_in_hash(
        points, [src, flow, :dest_count]) {|v| (v.nil? ? 0 : v)}
      update_in_hash(
        points, [dest, flow, :dest_count]) {|v| (v.nil? ? 1 : v + 1)}
      update_in_hash(
        points, [dest, flow, :source_count]) {|v| (v.nil? ? 0 : v)}
    end
    points.each do |loc, flow_map|
      flow_map.each do |flow, info|
        add_link_connect_points(
          info, outflow_points, loc, flow, true)
        add_link_connect_points(
          info, inflow_points, loc, flow, false)
      end
    end
    @network_link.each do |link|
      src = link.fetch(:source_location_id)
      tgt = link.fetch(:destination_location_id)
      flow = link.fetch(:flow)
      sc = points[src][flow][:source].shift
      tc = points[tgt][flow][:dest].shift
      add_connection(sc[0], sc[1], tc[0], tc[1], flow)
    end
  end

end
