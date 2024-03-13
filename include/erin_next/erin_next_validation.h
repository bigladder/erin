/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_NEXT_VALIDATION_H
#define ERIN_NEXT_VALIDATION_H
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace erin_next
{
	std::unordered_set<std::string> const ValidTimeUnits{
		"years",
		"year",
		"yr",
		"weeks",
		"week",
		"days",
		"day",
		"hours",
		"hour",
		"h",
		"minutes",
		"minute",
		"min",
		"seconds",
		"second",
		"s"
	};

	std::unordered_set<std::string> const ValidRateUnits{
		"W",
		"kW",
		"MW",
	};

	std::unordered_set<std::string> const ValidQuantityUnits{
		"J",
		"kJ",
		"MJ",
		"Wh",
		"kWh",
		"MWh",
	};

	struct TagWithDeprication
	{
		std::string Tag;
		bool IsDeprecated = false;
	};

	// TODO: add "types" from the user-manual:
	// - real>0
	// - frac
	// - frac>0
	// - str
	// - [str]
	// - [real]
	// - etc.
	enum class InputType
	{
		Any,
		// TODO: rename below to String
		AnyString,
		EnumString,
		Number, // float or integer
		Integer, // only integer; will also parse 3.0 as 3, though
		ArrayOfDouble,
		ArrayOfString,
		ArrayOfTuple3OfString,
		ArrayOfTuple2OfNumber,
		MapFromStringToString,
	};

	struct InputValue
	{
		InputType Type;
		std::variant<
			std::string,
			double,
			int64_t,
			std::vector<double>,
			std::vector<std::string>,
			std::vector<std::vector<std::string>>,
			std::vector<std::vector<double>>,
			std::unordered_map<std::string, std::string>> Value;
	};

	enum class InputSection
	{
		SimulationInfo,
		Loads_01Explicit,
		Loads_02FileBased,
		Components_ConstantLoad,
		Components_Load,
		Components_Source,
		Components_UncontrolledSource,
		Components_ConstEffConverter,
		Components_Mux,
		Components_Store,
		Components_PassThrough,
		Dist_Fixed,
		Network,
		Scenarios,
	};

	// TODO: add ability to write out all of these to markdown
	// for inclusion into the documentation. Consider adding a
	// description tag to FieldInfo.
	struct FieldInfo
	{
		std::string FieldName;
		InputType Type;
		bool IsRequired;
		std::string Default;
		std::unordered_set<std::string> EnumValues;
		std::vector<TagWithDeprication> Aliases;
		std::unordered_set<InputSection> Sections;
	};

	struct ValidationInfo
	{
		std::unordered_map<std::string, InputType> TypeMap;
		std::unordered_set<std::string> RequiredFields;
		std::unordered_set<std::string> OptionalFields;
		std::unordered_map<std::string, std::unordered_set<std::string>> EnumMap;
		std::unordered_map<std::string, std::string> Defaults;
		std::unordered_map<std::string, std::vector<TagWithDeprication>> Aliases;
	};

	struct ComponentValidationMap
	{
		ValidationInfo ConstantLoad;
		ValidationInfo ScheduleBasedLoad;
		ValidationInfo ConstantSource;
		ValidationInfo ScheduleBasedSource;
		ValidationInfo ConstantEfficiencyConverter;
		ValidationInfo Mux;
		ValidationInfo Store;
		ValidationInfo PassThrough;
	};

	struct InputValidationMap
	{
		ComponentValidationMap Comp;
		ValidationInfo Load_01Explicit;
		ValidationInfo Load_02FileBased;
		ValidationInfo Dist_Fixed;
		ValidationInfo SimulationInfo;
		ValidationInfo Network;
		ValidationInfo Scenario;
	};

	std::string
	InputSection_toString(InputSection s);

	std::optional<InputSection>
	String_toInputSection(std::string tag);

	void
	UpdateValidationInfoByField(ValidationInfo& info, FieldInfo const& f);

	InputValidationMap
	SetupGlobalValidationInfo();

}

#endif
