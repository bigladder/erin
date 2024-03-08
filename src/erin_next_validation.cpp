#include "erin_next/erin_next_validation.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <assert.h>

namespace erin_next
{

	std::string
	InputSection_toString(InputSection s)
  {
    switch (s)
    {
  		case InputSection::SimulationInfo:
      {
        return "simulation_info";
        
      } break;
  		case InputSection::Loads_01Explicit:
      {
        return "loads";
      } break;
  		case InputSection::Loads_02FileBased:
      {
        return "loads (file-based)";
      } break;
  		case InputSection::Components_Source:
      {
        return "components (source)";
        
      } break;
  		case InputSection::Components_ConstantLoad:
      {
        return "components (constant-load)";
      } break;
  		case InputSection::Components_Load:
      {
        return "components (load)";
      } break;
  		case InputSection::Dist_Fixed:
      {
        return "dist (fixed)";
      } break;
  		case InputSection::Network:
      {
        return "network";
      } break;
  		case InputSection::Scenarios:
      {
        return "scenarios";
      } break;
      default:
      {
      } break;
    }
    std::cerr << "Unhandled scenario" << std::endl;
    std::exit(1);
  }

	std::optional<InputSection>
	String_toInputSection(std::string tag)
  {
    if (tag == "simulation_info")
    {
      return InputSection::SimulationInfo;
    }
    if (tag == "loads")
    {
      return InputSection::Loads_01Explicit;
    }
    if (tag == "loads (file-based)")
    {
      return InputSection::Loads_02FileBased;
    }
    if (tag == "components (source)")
    {
      return InputSection::Components_Source;
    }
    if (tag == "components (constant-load)")
    {
      return InputSection::Components_ConstantLoad;
    }
    if (tag == "components (load)")
    {
      return InputSection::Components_Load;
    }
    if (tag == "dist (fixed)")
    {
      return InputSection::Dist_Fixed;
    }
    if (tag == "network")
    {
      return InputSection::Network;
    }
    if (tag == "scenarios")
    {
      return InputSection::Scenarios;
    }
    return {};
  }

	void
	UpdateValidationInfoByField(ValidationInfo& info, FieldInfo const& f)
  {
    assert(!info.TypeMap.contains(f.FieldName));
    info.TypeMap.insert({f.FieldName, f.Type});
    if (f.Type == InputType::EnumString)
    {
      assert(f.EnumValues.size() > 0);
      info.EnumMap.insert({f.FieldName, f.EnumValues});
    }
    if (f.IsRequired)
    {
      info.RequiredFields.insert(f.FieldName);
    }
    else
    {
      info.OptionalFields.insert(f.FieldName);
    }
    if (!f.Default.empty())
    {
      info.Defaults.insert({f.FieldName, f.Default});
    }
    if (f.Aliases.size() > 0)
    {
      info.Aliases.insert({f.FieldName, f.Aliases});
    }
  }

	InputValidationMap
	SetupGlobalValidationInfo()
  {
    std::unordered_set<InputSection> allSections{
  		InputSection::SimulationInfo,
  		InputSection::Loads_01Explicit,
  		InputSection::Loads_02FileBased,
  		InputSection::Components_Source,
  		InputSection::Components_ConstantLoad,
  		InputSection::Components_Load,
  		InputSection::Dist_Fixed,
  		InputSection::Network,
  		InputSection::Scenarios,
    };
    std::unordered_set<InputSection> allCompSections{
  		InputSection::Components_Source,
  		InputSection::Components_ConstantLoad,
  		InputSection::Components_Load,
    };
    std::unordered_set<InputSection> compSections{
  		InputSection::Components_Source,
  		InputSection::Components_ConstantLoad,
  		InputSection::Components_Load,
    };
    std::unordered_set<std::string> compTypeEnums{
      "source",
      "load",
      "store",
    };
    std::vector<FieldInfo> fields{
      // GLOBAL
      FieldInfo{
        .FieldName = "meta",
        .Type = InputType::Any,
        .IsRequired = false,
        .Default = "",
        .EnumValues = {},
        .Aliases = {},
        .Sections = allSections,
      },
      // SIMULATION_INFO
      FieldInfo{
        .FieldName = "rate_unit",
        .Type = InputType::EnumString,
        .IsRequired = false,
        .Default = "W",
        .EnumValues = ValidRateUnits,
        .Aliases = {},
        .Sections = {
          InputSection::SimulationInfo,
        },
      },
      FieldInfo{
        .FieldName = "quantity_unit",
        .Type = InputType::EnumString,
        .IsRequired = false,
        .Default = "J",
        .EnumValues = ValidQuantityUnits,
        .Aliases = {},
        .Sections = {
          InputSection::SimulationInfo,
        },
      },
      FieldInfo{
        .FieldName = "time_unit",
        .Type = InputType::EnumString,
        .IsRequired = false,
        .Default = "yr",
        .EnumValues = ValidTimeUnits,
        .Aliases = {},
        .Sections = {
          InputSection::SimulationInfo,
        },
      },
      FieldInfo{
        .FieldName = "max_time",
        .Type = InputType::Number,
        .IsRequired = false,
        .Default = "1000.0",
        .EnumValues = ValidTimeUnits,
        .Aliases = {},
        .Sections = {
          InputSection::SimulationInfo,
        },
      },
      FieldInfo{
        .FieldName = "random_seed",
        .Type = InputType::Integer,
        .IsRequired = false,
        .Default = "17",
        .EnumValues = ValidTimeUnits,
        .Aliases = {},
        .Sections = {
          InputSection::SimulationInfo,
        },
      },
      FieldInfo{
        .FieldName = "fixed_random",
        .Type = InputType::Number,
        .IsRequired = false,
        .Default = "",
        .EnumValues = {},
        .Aliases = {},
        .Sections = {
          InputSection::SimulationInfo,
        },
      },
      FieldInfo{
        .FieldName = "fixed_random_series",
        .Type = InputType::ArrayOfDouble,
        .IsRequired = false,
        .Default = "",
        .EnumValues = {},
        .Aliases = {},
        .Sections = {
          InputSection::SimulationInfo,
        },
      },
      // Loads -- File-Based
      FieldInfo{
        .FieldName = "csv_file",
        .Type = InputType::AnyString,
        .IsRequired = true,
        .Default = "",
        .EnumValues = {},
        .Aliases = {},
        .Sections = {
          InputSection::Loads_02FileBased,
        },
      },
      // Loads -- Explicit
      FieldInfo{
        .FieldName = "time_rate_pairs",
        .Type = InputType::ArrayOfTuple2OfNumber,
        .IsRequired = true,
        .Default = "",
        .EnumValues = {},
        .Aliases = {},
        .Sections = {
          InputSection::Loads_01Explicit,
        },
      },
      FieldInfo{
        .FieldName = "time_unit",
        .Type = InputType::EnumString,
        .IsRequired = false,
        .Default = "s",
        .EnumValues = ValidTimeUnits,
        .Aliases = {},
        .Sections = {
          InputSection::Loads_01Explicit,
        },
      },
      FieldInfo{
        .FieldName = "rate_unit",
        .Type = InputType::EnumString,
        .IsRequired = false,
        .Default = "W",
        .EnumValues = ValidRateUnits,
        .Aliases = {},
        .Sections = {
          InputSection::Loads_01Explicit,
        },
      },
      // Components -- Global
      FieldInfo{
        .FieldName = "type",
        .Type = InputType::EnumString,
        .IsRequired = true,
        .Default = "",
        .EnumValues = compTypeEnums,
        .Aliases = {},
        .Sections = compSections,
      },
    };
    InputValidationMap v{};
    for (auto const& f : fields)
    {
      if (f.Sections.size() == 0)
      {
        std::cerr << "Program Initialization Error: "
          << "field '" << f.FieldName << "' has no "
          << "sections that it applies to" << std::endl;
        std::exit(1);
      }
      for (auto const sec : f.Sections)
      {
        switch (sec)
        {
          case InputSection::SimulationInfo:
          {
            UpdateValidationInfoByField(v.SimulationInfo, f);
          } break;
          case InputSection::Loads_01Explicit:
          {
            UpdateValidationInfoByField(v.Load_01Explicit, f);
          } break;
          case InputSection::Loads_02FileBased:
          {
            UpdateValidationInfoByField(v.Load_02FileBased, f);
          } break;
          case InputSection::Components_ConstantLoad:
          {
            UpdateValidationInfoByField(v.Comp_ConstantLoad, f);
          } break;
          case InputSection::Components_Load:
          {
            UpdateValidationInfoByField(v.Comp_ScheduleBasedLoad, f);
          } break;
          case InputSection::Components_Source:
          {
            UpdateValidationInfoByField(v.Comp_ConstantSource, f);
          } break;
          case InputSection::Components_ScheduleBasedSource:
          {
            UpdateValidationInfoByField(v.Comp_ScheduleBasedSource, f);
          } break;
          case InputSection::Dist_Fixed:
          {
            UpdateValidationInfoByField(v.Dist_Fixed, f);
          } break;
          case InputSection::Network:
          {
            UpdateValidationInfoByField(v.Network, f);
          } break;
          case InputSection::Scenarios:
          {
            UpdateValidationInfoByField(v.Scenario, f);
          } break;
          default:
          {
            std::cerr << "Program Initialization Error: "
              << "unhandled section '"
              << InputSection_toString(sec) << "'" << std::endl;
            std::exit(1);
          } break;
        }
      }
    }
    return v;
  }
  
}
