#include "erin_next/erin_next_component.h"
#include "erin_next/erin_next_toml.h"

namespace erin_next
{
	bool
	ParseSingleComponent(
		Simulation& s,
		Model& m,
		toml::table const& table,
		std::string const& tag)
	{
		std::string fullTableName = "components." + tag;
		if (!table.contains("type"))
		{
			std::cout << "[" << fullTableName << "] "
				<< "required field 'type' not present" << std::endl;
			return false;
		}
		std::optional<ComponentType> maybeCompType =
			TagToComponentType(table.at("type").as_string());
		if (!maybeCompType.has_value())
		{
			std::cout << "[" << fullTableName << "] "
				<< "unable to parse table type from '"
				<< table.at("type").as_string() << "'" << std::endl;
			return false;
		}
		ComponentType ct = maybeCompType.value();
		size_t id = {};
		std::string inflow{};
		size_t inflowId = {};
		std::string outflow{};
		size_t outflowId = {};
		if (table.contains("outflow"))
		{
			auto maybe = TOMLTable_ParseString(table, "outflow", fullTableName);
			if (maybe.has_value())
			{
				outflow = maybe.value();
				outflowId = Simulation_RegisterFlow(s, outflow);
			}
		}
		if (table.contains("inflow"))
		{
			auto maybe = TOMLTable_ParseString(table, "inflow", fullTableName);
			if (maybe.has_value())
			{
				inflow = maybe.value();
				inflowId = Simulation_RegisterFlow(s, inflow);
			}
		}
		switch (ct)
		{
			case (ComponentType::ConstantSourceType):
			{
				// TODO: need to get the max available from constant source
				// for now, we can use uint32_t max?
				id = Model_AddConstantSource(
					m, std::numeric_limits<uint32_t>::max());
			} break;
			case (ComponentType::ScheduleBasedLoadType):
			{
				// TODO: how do we handle that loads are by scenario?
				// A model, however, **is** the run for a single scenario...
				// id = Model_AddScheduleBasedLoad(...);
			} break;
			default:
			{
				throw new std::runtime_error{ "Unhandled component type" };
			}
		}
		return true;
	}

	bool
	ParseComponents(Simulation& s, Model& m, toml::table const& table)
	{
		for (auto it = table.cbegin(); it != table.cend(); ++it)
		{
			auto wasSuccessful =
				ParseSingleComponent(s, m, it->second.as_table(), it->first);
			if (!wasSuccessful)
			{
				return false;
			}
		}
		return true;
	}
}