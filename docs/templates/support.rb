require 'set'
require 'csv'

class Support
  # - data: (hash symbol various), a hash table with keys
  #   - :component_failure_mode, an array of Hash with keys:
  #     - :component_id, string, the component id to apply to
  #     - :failure_mode_id, string, the failure mode to apply
  #   - :component_fragility, an array of Hash with keys:
  #     - :component_id, string, the component to apply the fragility curve to
  #     - :fragility_id, string, the fragility curve id to be applied
  #   - :converter_component, an array of Hash with keys:
  #     - :id, string, id of the converter_component
  #     - :location_id, string, the location of the converter
  #     - :inflow, string, the inflow type (e.g., "natural_gas", "diesel", etc.)
  #     - :outflow, string, the outflow type (e.g., "electricity", "heating", etc.)
  #     - :lossflow, string, optional. defaults to "waste_heat". The lossflow stream
  #     - :constant_efficiency, number, where 0.0 < efficiency <= 1.0, the efficiency of conversion
  #   - :damage_intensity, an array of Hash with keys:
  #     - :scenario_id, the scenario that the damage intensity applies to
  #     - :name, string, the name of the damage metric (e.g., "wind_speed_mph",
  #       "inundation_depth_ft", etc.)
  #     - :value, number, the value of the damage metric (e.g., 150, 12, etc.)
  #   - :dual_outflow_converter_comp
  #     - :location_id, string, the location of the dual-outflow converter
  #     - :inflow, string, the inflow type (e.g., "natural_gas", "diesel", "coal", etc.)
  #     - :primary_outflow, string, the primary outflow type (e.g., "electricity", "heating")
  #     - :secondary_outflow, string, the secondary outflow type (e.g., "heating", "electricity")
  #     - :lossflow, string, optional. defaults to "waste_heat"
  #     - :primary_efficiency, number, 0.0 < efficiency <= 1.0
  #     - :secondary_efficiency, number, 0.0 < efficiency <= 1.0
  #   - :failure_mode, (Array Hash) with keys for Hash as follows:
  #     - :id, string, the id of the failure mode
  #     - :failure_dist, string, the id of the failure CDF
  #     - :repair_dist, string, the id of the repair CDF
  #   - :fixed_dist, (Array (Hash symbol value)) with these symbols
  #     - :id, string, the id of the fixed cdf
  #     - :value_in_hours, number, the fixed value in hours
  #   - :fragility_curve, (Array (Hash symbol value)) with these symbols
  #     - :id, string, the id of the fragility curve
  #     - :vulnerable_to, string, the damage intensity vulnerable to
  #     - :lower_bound, number, the value outside of which no damage occurrs
  #     - :upper_bound, number, the value outside of which certain destruction occurs
  #   - :general, (Hash symbol value)
  #     - :simulation_duration_in_years, number, the duration in years
  #     - :random_setting, string, one of #{"Auto", "Seed"}
  #     - :random_seed, integer, the random seed
  #   - :load_component, an array of Hash with keys:
  #     - :location_id, string, the location of the load
  #     - :inflow, string, the inflow type (e.g., 'electricity', 'heating',
  #       'cooling', 'natural_gas', etc.)
  #   - :load_profile, an array of Hash with keys:
  #     - :scenario_id, string, the id of the scenario
  #     - :component_id, string, id of a load_component or uncontrolled_src
  #     - :flow, string, the flow for the load profile
  #     - :file, string, path to the csv file to load
  #   - :mover_component, an array of Hash with keys:
  #     - :id, string, the id of the mover
  #     - :location_id, string, the location of the mover
  #     - :flow_moved: string, the main flow being moved (e.g., "heating", "heat_removed", etc.)
  #     - :support_flow: string, the support inflow required to do the moving (e.g., "electricity")
  #     - :cop, number, the coefficient of performance relating the inflow_moved with inflow_support
  #   - :muxer_component, an array of Hash with keys:
  #     - :location_id, string, the location of the muxer
  #     - :flow, string, the type of flow through the muxer (e.g., 'electricity', 'natural_gas', 'coal', etc.)
  #     - :num_inflows, integer, number > 0, the number of inflow ports
  #     - :num_outflows, integer, number > 0, the number of outflow ports
  #   - :network_link, an array of Hash with keys:
  #     - :source_location_id, string, location_id for the source of the flow
  #     - :destination_location_id, string, location_id for the destination of the flow
  #     - :flow, string, the type of flow (e.g., 'electricity', 'heating', etc.)
  #   - :pass_through_component, an array of Hash with keys:
  #     - :id, string, id of the pass-through component
  #     - :link_id, string, the network link having the pass-through component
  #     - :flow, string, the flow being passed through (e.g., 'electricity', 'diesel')
  #   - :scenario, an array of Hash with keys:
  #     - :id, string, scenario id
  #     - :duration_in_hours, number, duration
  #     - :occurrence_distribution, string, id of the distribution to use
  #     - :calc_reliability, boolean string ("TRUE" or "FALSE")
  #     - :max_occurrence, integer, -1 = unlimited; otherwise, the max number
  #       of occurrences
  #   - :source_component, an array of Hash with keys:
  #     - :location_id, string, the location of the source
  #     - :outflow, string, the outflow provided (e.g., 'electricity', 'natural_gas', 'diesel')
  #     - :is_limited, boolean string ("TRUE" or "FALSE"), whether or not the source is limited
  #     - :max_outflow_kW, number, the maximum outflow in kW
  #   - :storage_component, an array of Hash with keys:
  #     - :id, string, optional id
  #     - :location_id, string, the location of the storage
  #     - :flow, string, the flow being stored (e.g., "electricity", "diesel", etc.)
  #     - :capacity_kWh, number, > 0, the capacity of the store in kWh
  #     - :max_inflow_kW, number, >= 0, the maximum inflow to the storage in kW
  #   - :uncontrolled_src, an array of Hash with keys:
  #     - :id, string, id of the uncontrolled source
  #     - :location_id, string, the location of the uncontrolled source
  #     - :outflow, string, the flow from the uncontrolled source

  attr_reader(
    :component_failure_mode,
    :component_fragility,
    :converter_component,
    :damage_intensity,
    :failure_mode,
    :dist_type,
    :fixed_dist,
    :fragility_curve,
    :load_component,
    :load_profile,
    :mover_component,
    :muxer_component,
    :network_link,
    :pass_through_component,
    :scenario,
    :source_component,
    :storage_component,
    :uncontrolled_src,
  )

  def initialize(data, root_path=nil)
    @ids_in_use = Set.new
    @component_failure_mode = data.fetch(:component_failure_mode, [])
    @component_fragility = data.fetch(:component_fragility, [])
    @converter_component = data.fetch(:converter_component, [])
    @damage_intensity = data.fetch(:damage_intensity, [])
    @failure_mode = data.fetch(:failure_mode, [])
    @dist_type = data.fetch(:dist_type, [])
    @fixed_dist = data.fetch(:fixed_dist, [])
    @fragility_curve = data.fetch(:fragility_curve, [])
    @load_component = data.fetch(:load_component, [])
    @load_profile = data.fetch(:load_profile, [])
    @mover_component = data.fetch(:mover_component, [])
    @muxer_component = data.fetch(:muxer_component, [])
    @network_link = data.fetch(:network_link, [])
    @pass_through_component = data.fetch(:pass_through_component, [])
    @scenario = data.fetch(:scenario, [])
    @source_component = data.fetch(:source_component, [])
    @storage_component = data.fetch(:storage_component, [])
    @uncontrolled_src = data.fetch(:uncontrolled_src, [])
    process_cdfs
    expand_load_profile_paths(root_path) unless root_path.nil?
    ensure_components_have_ids
    @connections = []
    process_dual_outflow_converter_comp(
      data.fetch(:dual_outflow_converter_comp, []))
    generate_connections
  end

  def self.get_script_directory
    __dir__
  end

  def connections
    @connections.sort
  end

  def cdf_for_id(id)
    item = nil
    @fixed_dist.each do |cdf|
      if id == cdf[:id]
        item = cdf 
        break
      end
    end
    item
  end

  def damage_intensities_for_scenario(scenario_id)
    @damage_intensity.select {|di| di[:scenario_id] == scenario_id}
  end

  def fragilities_for_component(comp_id)
    frag_ids = Set.new
    @component_fragility.each do |cf|
      frag_ids << cf[:fragility_id] if cf[:component_id] == comp_id
    end
    frags = []
    @fragility_curve.each do |fc|
      frags << fc if frag_ids.include?(fc[:id])
    end
    frags
  end

  def failure_modes_for_component(comp_id)
    fm_ids = Set.new
    @component_failure_mode.each do |cfm|
      fm_ids << cfm[:failure_mode_id] if cfm[:component_id] == comp_id
    end
    fms = []
    @failure_mode.each do |fm|
      fms << fm if fm_ids.include?(fm[:id])
    end
    fms
  end

  def pass_through_components_for_link(link_id)
    pts = []
    @pass_through_component.each do |pt|
      pts << pt if pt.fetch(:link_id) == link_id 
    end
    pts
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
          if hr[1].nil?
            map[hr[0]] = ""
          else
            map[hr[0]] = hr[1].strip
          end
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

  def add_converter_component(eff, loc, inflow, outflow, lossflow="waste_heat", id=nil)
    id ||= make_id_unique("#{loc}_#{outflow}_generator")
    @converter_component << {
      id: id,
      location_id: loc,
      inflow: inflow,
      outflow: outflow,
      lossflow: lossflow,
      constant_efficiency: eff,
    }
    id
  end

  def process_dual_outflow_converter_comp(dual_outflow_comps)
    dual_outflow_comps.each do |c|
      id = c.fetch(
        :id,
        "#{c.fetch(:location_id)}_dual_"\
        "#{c.fetch(:primary_outflow)}_"\
        "#{c.fetch(:secondary_outflow)}_generator")
      inf = c.fetch(:inflow)
      out1 = c.fetch(:primary_outflow)
      out2 = c.fetch(:secondary_outflow)
      loss = c.fetch(:lossflow, "waste_heat")
      cc1 = add_converter_component(
        c.fetch(:primary_efficiency),
        c.fetch(:location_id),
        inf, out1, loss, id + "_stage_1")
      cc2 = add_converter_component(
        c.fetch(:secondary_efficiency),
        c.fetch(:location_id),
        loss, out2, loss, id + "_stage_2")
      add_connection(cc1, 1, cc2, 0, loss)
    end
  end

  # TODO: rename to process_distributions; but do we even need this anymore?
  def process_cdfs
    @fixed_dist.each do |cdf|
      cdf[:type] = "fixed"
    end
  end

  def expand_load_profile_paths(root_path)
    @load_profile.each do |lp|
      bn = File.basename(lp[:file])
      lp[:file] = File.expand_path(File.join(root_path, bn))
    end
  end

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
      [@converter_component, [:location_id, :outflow], "generator"],
      [@load_component, [:location_id, :inflow], ""],
      [@load_profile, [:component_id, :scenario_id], ""],
      [@mover_component, [:location_id, :flow_moved], "mover"],
      [@network_link, [:source_location_id, "to", :destination_location_id, :flow], ""],
      [@source_component, [:location_id, :outflow], "source"],
      [@storage_component, [:location_id, :flow], "store"],
      [@uncontrolled_src, [:location_id, :outflow], "uncontrolled"],
    ]
    # lp[:id] = "#{lp[:building_id]}_#{lp[:enduse]}_#{lp[:scenario_id]}" %>
    comp_infos.each do |tuple|
      cs, attribs, postfix = tuple
      cs.each do |c|
        next if c.include?(:id)
        parts = []
        attribs.each do |a|
          if a.is_a?(String)
            parts << a
          else
            parts << c.fetch(a).to_s.strip
          end
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
  def add_muxer_component(location_id, flow, num_inflows, num_outflows, id=nil)
    id ||= make_id_unique("#{location_id}_#{flow}_bus")
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
      @converter_component,
      @load_component,
      @mover_component,
      @muxer_component,
      @source_component,
      @storage_component,
      @uncontrolled_src,
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
      [@mover_component, [:location_id], [:flow_moved, :support_flow]],
      [@network_link, [:source_location_id, :destination_location_id], [:flow]],
      [@uncontrolled_src, [:location_id], [:outflow]],
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
    loads_0 = get_conn_info_by_location_and_flow(
      loc, flow,
      @converter_component, :location_id, :inflow, [inflow_port])
    support_inport = 1
    loads_1 = get_conn_info_by_location_and_flow(
      loc, flow,
      @mover_component, :location_id, :support_flow, [support_inport])
    loads_0 + loads_1
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

  def get_movers(loc, outflow)
    outflow_port = 0
    get_conn_info_by_location_and_flow(
      loc, outflow,
      @mover_component, :location_id, :flow_moved, [outflow_port])
  end

  def get_uncontrolled_sources(loc, flow)
    outflow_port = 0
    get_conn_info_by_location_and_flow(
      loc, flow,
      @uncontrolled_src, :location_id, :outflow, [outflow_port])
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

  # - comp_bundles: (list (list (or component-conneciton-info symbol)))
  # - loc: string, id of the location
  # - flow: string, id of the flow
  # - inflow_points: Hash, inflow points for inter-location connections
  # - outflow_points: Hash, outflow points for inter-location connections
  # RETURN: nil
  # NOTE: component-connection-info: (tuple id port)
  # Adds muxers and connects everything up and returns a hash by symbol with connect info
  def connect_in_series(comp_bundles, loc, flow, inflow_points, outflow_points)
    bs = comp_bundles.reject {|b| b.empty?}
    if bs.length > 1
      bs[0..-2].zip(bs[1..-1]).each do |outflows, inflows|
        num_outflows = outflows.length
        num_inflows = inflows.length
        if num_outflows == 1 and num_inflows == 1
          if (inflows[0] == :inflow_port) and (outflows[0] == :outflow_port)
            throw "unsupported operation"
          elsif inflows[0] == :inflow_port
            assign_in_hash(inflow_points, [loc, flow], outflows[0])
          elsif outflows[0] == :outflow_port
            assign_in_hash(outflow_points, [loc, flow], inflows[0])
          else
            inflow_id, inflow_port = inflows[0]
            outflow_id, outflow_port = outflows[0]
            add_connection(inflow_id, inflow_port, outflow_id, outflow_port, flow)
          end
        elsif num_outflows > 0 and num_inflows > 0
          bus = add_muxer_component(loc, flow, num_inflows, num_outflows)
          outflows.each_with_index do |outf, idx|
            if outf == :outflow_port
              assign_in_hash(outflow_points, [loc, flow], [bus, idx])
            else
              outflow_id, outflow_port = outf
              add_connection(bus, idx, outflow_id, outflow_port, flow)
            end
          end
          inflows.each_with_index do |inf, idx|
            if inf == :inflow_port
              assign_in_hash(inflow_points, [loc, flow], [bus, idx])
            else
              inflow_id, inflow_port = inf
              add_connection(inflow_id, inflow_port, bus, idx, flow)
            end
          end
        else
          raise "should not be possible"
        end
      end
    end
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
        movers = get_movers(loc, flow)
        ucs = get_uncontrolled_sources(loc, flow)
        num_outbound = outbound_links.length
        num_inbound = inbound_links.length
        bundles = []
        bundles << loads + internal_loads + (num_outbound > 0 ? [:outflow_port] : [])
        bundles << stores
        bundles << movers
        bundles << sources + ucs + (num_inbound > 0 ? [:inflow_port] : []) + converters
        connect_in_series(bundles, loc, flow, inflow_points, outflow_points)
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
      link_id = link.fetch(:id)
      flow = link.fetch(:flow)
      pts = pass_through_components_for_link(link_id)
      num_pts = pts.length
      begin
        sc = points[src][flow][:source].shift
        tc = points[tgt][flow][:dest].shift
        if pts.empty?
          add_connection(sc[0], sc[1], tc[0], tc[1], flow)
        elsif num_pts == 1
          pt = pts[0]
          pt_outflow_port = 0
          pt_inflow_port = 0
          pt_id = pt.fetch(:id)
          add_connection(sc[0], sc[1], pt_id, pt_inflow_port, flow)
          add_connection(pt_id, pt_outflow_port, tc[0], tc[1], flow)
        else
          inflow_bus = add_muxer_component(
            link_id, flow, 1, num_pts, "#{link_id}_inflow_bus")
          outflow_bus = add_muxer_component(
            link_id, flow, num_pts, 1, "#{link_id}_outflow_bus")
          add_connection(sc[0], sc[1], inflow_bus, 0, flow)
          pts.each_with_index do |pt, port|
            add_connection(inflow_bus, port, pt.fetch(:id), 0, flow)
            add_connection(pt.fetch(:id), 0, outflow_bus, port, flow)
          end
          add_connection(outflow_bus, 0, tc[0], tc[1], flow)
        end
      rescue
        puts "WARNING! Bad Connection"
        puts "Trying to connect '#{src}' to '#{tgt}' with '#{flow}' "\
          "but insufficient connection information"
      end
    end
  end
end
