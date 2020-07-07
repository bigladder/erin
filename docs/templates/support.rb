require 'set'

class Support
  def initialize(load_profile_scenario_id,
                 load_profile_building_id,
                 load_profile_enduse,
                 load_profile_file)
    @load_profile_scenario_id = load_profile_scenario_id
    @load_profile_building_id = load_profile_building_id
    @load_profile_enduse = load_profile_enduse
    @load_profile_file = load_profile_file
    _check_load_profile_data
  end

  # INTERNAL METHOD
  def _check_load_profile_data
    num = @load_profile_building_id.length
    warns = []
    if num != @load_profile_scenario_id.length
      warns << ("load_profile_building_id.length (#{num}) != "\
                "load_profile_scenario_id.length "\
                "(#{@load_profile_scenario_id.length})")
    end
    if num != @load_profile_enduse.length
      warns << ("load_profile_building_id.length (#{num}) != "\
                "load_profile_enduse.length (#{@load_profile_enduse.length})")
    end
    if num != @load_profile_file.length
      warns << ("building_ids.length (#{num}) != "\
                "load_profile_file.length (#{@load_profile_file.length})")
    end
    warns.each do |w|
      puts "WARNING! #{w}"
    end
    nil
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

  #
  #def create_building_config(building_ids, egen_flags, egen_eff_pcts, heat_storage_flags, heat_storage_caps, gas_boiler_flags, 
  #  building_config = {}
  #  building_level_building_id.each_with_index do |b_id, n|                     
  #    if building_config.include?(b_id)                                         
  #    end                                                                       
  #    building_config[b_id] = {                                                 
  #      has_egen: is_true(building_level_egen_flag[n]),                         
  #      egen_eff: building_level_egen_eff_pct[n].to_f,                          
  #      has_tes: is_true(building_level_heat_storage_flag[n]),                  
  #      tes_cap_kWh: building_level_heat_storage_cap_kWh[n].to_f,               
  #      has_boiler: is_true(building_level_gas_boiler_flag[n]),                 
  #      boiler_eff: building_level_gas_boiler_eff_pct[n].to_f / 100.0,          
  #      e_supply_node: building_level_electricity_supply_node[n].strip,         
  #      ng_supply_node: "utility",                                              
  #      enduses: Set.new,                                                       
  #    }                                                                         
  #end                                                                         

  #
  def create_node_config
  end
  #
  def generate_connections
  end
  #
  def generate_components
  end
end
