require 'set'

module Support
  # - building_ids: (Array string), array of building ids
  # - scenario_ids: (Array string), array of scenario ids
  # - enduses: (Array string), array of enduses
  # - load_profiles: (Array string), array of paths to csv files with the loads
  # RETURN: {
  #   :warnings => (Array string), any warnings found
  #   :loads => (Array {:load_id=>string, :file=>string}), the load data
  #   :load_ids => (Array string), the load ids used
  #   }
  def self.process_load_data(building_ids, scenario_ids, enduses, load_profiles)
    loads = []
    warns = []
    load_id_set = Set.new
    load_ids = []
    num = building_ids.length
    if num != scenario_ids.length
      warns << ("building_ids.length (#{num}) != "\
                "scenario_ids.length (#{scenario_ids.length})")
    elsif num != enduses.length
      warns << ("building_ids.length (#{num}) != "\
                "enduses.length (#{enduses.length})")
    elsif num != load_profiles.length
      warns << ("building_ids.length (#{num}) != "\
                "load_profiles.length (#{load_profiles.length})")
    end
    if warns.length > 0
      return {
        warnings: warns,
        loads: loads,
      }
    end
    building_ids.each_with_index do |b_id, n|
      s_id = scenario_ids[n]
      enduse = enduses[n]
      file = load_profiles[n]
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
end
