/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin_next/erin_next_load.h"
#include "erin_next/erin_next.h"
#include "erin_next/erin_next_units.h"
#include "erin_next/erin_next_validation.h"
#include "erin_next/erin_next_toml.h"
#include "erin_next/erin_next_csv.h"
#include "erin_next/erin_next_utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

namespace erin
{
    std::unordered_set<std::string> RequiredLoadFields{
        "time_unit",
        "rate_unit",
        "time_rate_pairs",
    };

    std::unordered_set<std::string> RequiredLoadFieldsCsv{"csv_file"};

    std::unordered_set<std::string> OptionalLoadFields{};

    std::unordered_map<std::string, std::string> DefaultLoadFields{};

    std::optional<Load>
    ParseSingleLoadExplicit(
        std::unordered_map<std::string, InputValue> const& table,
        std::string const& tag
    )
    {
        std::string tableFullName = "loads." + tag;
        TimeUnit timeUnit = TimeUnit::Second;
        PowerUnit rateUnit = PowerUnit::Watt;
        if (table.contains("time_unit"))
        {
            auto const& maybeTimeUnitStr =
                std::get<std::string>(table.at("time_unit").Value);
            auto maybe = TagToTimeUnit(maybeTimeUnitStr);
            if (!maybe.has_value())
            {
                WriteErrorMessage(
                    tableFullName,
                    "unhandled time_unit '" + maybeTimeUnitStr + "'"
                );
                return {};
            }
            timeUnit = maybe.value();
        }
        if (table.contains("rate_unit"))
        {
            auto rateUnitStr =
                std::get<std::string>(table.at("rate_unit").Value);
            auto maybe = TagToPowerUnit(rateUnitStr);
            if (!maybe.has_value())
            {
                WriteErrorMessage(
                    tableFullName, "unhandled rate_unit '" + rateUnitStr + "'"
                );
                return {};
            }
            rateUnit = maybe.value();
        }
        auto timeRatePairs = ConvertToTimeAndAmounts(
            std::get<std::vector<std::vector<double>>>(
                table.at("time_rate_pairs").Value
            ),
            Time_ToSeconds(1.0, timeUnit),
            Power_ToWatt(1.0, rateUnit)
        );
        Load load{
            .Tag = tag,
            .TimeAndLoads = std::move(timeRatePairs),
        };
        return load;
    }

    std::optional<Load>
    ParseSingleLoadFileLoad(
        std::unordered_map<std::string, InputValue> const& table,
        std::string const& tag
    )
    {
        std::string tableFullName = "loads." + tag;
        TimeUnit timeUnit = TimeUnit::Second;
        PowerUnit rateUnit = PowerUnit::Watt;
        std::vector<TimeAndAmount> timeRatePairs;
        auto csvFileName = std::get<std::string>(table.at("csv_file").Value);
        std::ifstream inputDataFile{};
        inputDataFile.open(csvFileName);
        if (!inputDataFile.good())
        {
            WriteErrorMessage(
                tableFullName,
                "unable to load input csv file '" + csvFileName + "'"
            );
            return {};
        }
        auto header = read_row(inputDataFile);
        if (header.size() == 2)
        {
            std::string const& timeUnitStr = header[0];
            std::string const& rateUnitStr = header[1];
            auto maybeTimeUnit = TagToTimeUnit(timeUnitStr);
            if (!maybeTimeUnit.has_value())
            {
                WriteErrorMessage(
                    tableFullName, "unhandled time unit: " + timeUnitStr
                );
                return {};
            }
            timeUnit = maybeTimeUnit.value();
            auto maybeRateUnit = TagToPowerUnit(rateUnitStr);
            if (!maybeRateUnit.has_value())
            {
                WriteErrorMessage(
                    tableFullName, "unhandled rate unit: " + rateUnitStr
                );
                return {};
            }
            rateUnit = maybeRateUnit.value();
        }
        else
        {
            WriteErrorMessage(
                tableFullName,
                "csv file '" + csvFileName
                    + "'"
                      " -- header must have 2 columns: time unit "
                      "and rate unit"
            );
            return {};
        }
        flow_t rowIdx = 1;
        std::vector<TimeAndAmount> trps{};
        while (inputDataFile.is_open() && inputDataFile.good())
        {
            std::vector<std::string> pair = read_row(inputDataFile);
            if (pair.size() == 0)
            {
                break;
            }
            ++rowIdx;
            if (pair.size() != 2)
            {
                WriteErrorMessage(
                    tableFullName,
                    "csv file '" + csvFileName
                        + "'"
                          " row: "
                        + std::to_string(rowIdx)
                        + "; must have 2 columns; "
                          "found: "
                        + std::to_string(pair.size())
                );
                return {};
            }
            TimeAndAmount ta{};
            ta.Time_s = Time_ToSeconds(std::stod(pair[0]), timeUnit);
            ta.Amount_W = static_cast<flow_t>(
                std::round(Power_ToWatt(std::stod(pair[1]), rateUnit))
            );
            trps.push_back(ta);
        }
        inputDataFile.close();
        timeRatePairs = trps;
        Load load{};
        load.Tag = tag;
        load.TimeAndLoads = std::move(timeRatePairs);
        return load;
    }

    std::vector<std::optional<Load>>
    ParseMultiLoadFileLoad(toml::table const& table, std::string const& tag)
    {
        std::string tableFullName = "loads." + tag;
        std::string csvFileName = table.at("multi_part_csv").as_string();
        std::ifstream inputDataFile{};
        inputDataFile.open(csvFileName);
        if (!inputDataFile.good())
        {
            WriteErrorMessage(
                tableFullName,
                "unable to load input csv file '" + csvFileName + "'"
            );
            return {};
        }

        struct LoadEntry
        {
            std::string name;
            std::size_t nItems;
            TimeUnit timeUnit;
            PowerUnit rateUnit;
        };

        std::vector<std::optional<Load>> loads;

        // read first row [name, size]
        std::vector<LoadEntry> loadEntries = {};
        auto sRow = read_row(inputDataFile);
        std::size_t nCols = sRow.size();
        for (std::size_t iCol = 0; iCol < nCols; iCol += 2)
        {
            if (iCol + 1 < nCols)
            {
                LoadEntry loadEntry;
                loadEntry.name = sRow[iCol];
                loadEntry.nItems = stoi(sRow[iCol + 1]);
                // TODO(mok): should actually read and parse in case different
                loadEntry.timeUnit = TimeUnit::Second;
                loadEntry.rateUnit = PowerUnit::Watt;
                loadEntries.push_back(loadEntry);
            }
        }
        // read second row [time unit, energy unit]
        std::vector<TimeAndAmount> timeRatePairs;

        sRow = read_row(inputDataFile);
        if (sRow.size() != nCols)
        {
            WriteErrorMessage(tableFullName, "invalid table size.");
            return {};
        }

        std::size_t iEntry = 0;
        for (std::size_t iCol = 0; iCol < nCols; iCol += 2)
        {
            if (iCol + 1 < nCols)
            {
                std::string const& timeUnitStr = sRow[iCol];
                std::string const& rateUnitStr = sRow[iCol + 1];
                auto maybeTimeUnit = TagToTimeUnit(timeUnitStr);
                if (!maybeTimeUnit.has_value())
                {
                    WriteErrorMessage(
                        tableFullName, "unhandled time unit: " + timeUnitStr
                    );
                    return {};
                }
                loadEntries[iEntry].timeUnit = maybeTimeUnit.value();
                auto maybeRateUnit = TagToPowerUnit(rateUnitStr);
                if (!maybeRateUnit.has_value())
                {
                    WriteErrorMessage(
                        tableFullName, "unhandled rate unit: " + rateUnitStr
                    );
                    return {};
                }
                loadEntries[iEntry].rateUnit = maybeRateUnit.value();
                ++iEntry;
            }
            else
            {
                WriteErrorMessage(
                    tableFullName,
                    "multi-part csv file '" + csvFileName
                        + "'"
                          " -- header 2nd row must have 2 columns for each "
                          "load entry: time unit "
                          "and rate unit"
                );
                return {};
            }
        }

        loads.reserve(loadEntries.size());
        for (auto& loadEntry : loadEntries)
        {
            Load load;
            load.Tag = loadEntry.name;
            load.TimeAndLoads = {};
            load.TimeAndLoads.reserve(loadEntry.nItems);
            loads.push_back(load);
        }
        std::size_t iRow = 0;
        do
        {
            sRow = read_row(inputDataFile);
            if (sRow.size() < 2 * loadEntries.size())
            {
                break;
            }
            std::size_t iCol = 0;
            std::size_t iLoad = 0;
            for (auto& loadEntry : loadEntries)
            {
                if (iRow < loadEntry.nItems)
                {
                    TimeAndAmount ta{};
                    ta.Time_s = Time_ToSeconds(
                        std::stod(sRow[iCol]), loadEntry.timeUnit
                    );
                    ta.Amount_W = static_cast<flow_t>(Power_ToWatt(
                        std::stod(sRow[iCol + 1]), loadEntry.rateUnit
                    ));
                    loads[iLoad]->TimeAndLoads.push_back(ta);
                }
                iCol += 2;
                ++iLoad;
            }
            ++iRow;
        } while (!inputDataFile.eof());
        inputDataFile.close();

        return loads;
    }

    std::optional<Load>
    ParseSingleLoad(
        std::string const& tag,
        toml::table const& table,
        std::string const& tableName,
        ValidationInfo const& explicitValidation,
        ValidationInfo const& fileValidation
    )
    {
        std::vector<std::string> errors01;
        std::vector<std::string> warnings01;
        auto const& explicitLoadTable = TOMLTable_ParseWithValidation(
            table, explicitValidation, tableName, errors01, warnings01
        );
        std::optional<Load> maybeLoad = {};
        if (errors01.size() == 0)
        {
            maybeLoad = ParseSingleLoadExplicit(explicitLoadTable, tag);
            if (!maybeLoad.has_value())
            {
                WriteErrorMessage(tableName, "unable to load");
                return {};
            }
            if (warnings01.size() > 0)
            {
                for (auto const& w : warnings01)
                {
                    WriteErrorMessage(tableName, w);
                }
            }
        }
        else
        {
            std::vector<std::string> errors02;
            std::vector<std::string> warnings02;
            auto const& fileLoadTable = TOMLTable_ParseWithValidation(
                table, fileValidation, tableName, errors02, warnings02
            );
            if (errors02.size() > 0)
            {
                WriteErrorMessage(
                    tableName, "unable to load explicitly or by file"
                );
                for (auto const& err : errors01)
                {
                    WriteErrorMessage(tableName, err);
                }
                for (auto const& err : errors02)
                {
                    WriteErrorMessage(tableName, err);
                }
                return {};
            }
            if (warnings02.size() > 0)
            {
                for (auto const& w : warnings02)
                {
                    WriteWarningMessage(tableName, w);
                }
            }
            maybeLoad = ParseSingleLoadFileLoad(fileLoadTable, tag);
            if (!maybeLoad.has_value())
            {
                WriteErrorMessage(tableName, "unable to load");
                return {};
            }
        }
        return maybeLoad;
    }

    std::vector<std::optional<Load>>
    ParseMultiLoad(std::string const& tableName, toml::table const& table)
    {

        std::vector<std::optional<Load>> maybeLoads = {};
        maybeLoads = ParseMultiLoadFileLoad(table, tableName);
        for (auto const& maybeLoad : maybeLoads)
        {
            if (!maybeLoad.has_value())
            {
                WriteErrorMessage(tableName, "unable to load");
                return {};
            }
        }

        return maybeLoads;
    }

    bool
    MaybePushLoad(
        std::optional<Load> maybeLoad,
        const std::string& tableName,
        std::vector<Load>& loads
    )
    {
        bool loadGood = true;
        for (auto& load : loads)
        {
            if (load.Tag == maybeLoad->Tag)
            {
                WriteErrorMessage(
                    tableName, "load " + maybeLoad->Tag + " already exists"
                );
                loadGood = false;
                break;
            }
        }
        if (loadGood)
        {
            loads.push_back(std::move(maybeLoad.value()));
        }
        return loadGood;
    }

    std::optional<std::vector<Load>>
    ParseLoads(
        toml::table const& table,
        ValidationInfo const& explicitValidation,
        ValidationInfo const& fileValidation,
        Log const& log
    )
    {
        std::vector<Load> loads{};
        loads.reserve(table.size());
        for (auto it = table.cbegin(); it != table.cend(); ++it)
        {
            std::string tag = it->first;
            std::string tableName = "loads." + tag;
            if (it->second.is_table())
            {
                toml::table const& table2 = it->second.as_table();
                std::vector<std::string> errors01;
                std::vector<std::string> warnings01;
                auto const& explicitLoadTable = TOMLTable_ParseWithValidation(
                    table2, explicitValidation, tableName, errors01, warnings01
                );
                std::optional<Load> maybeLoad;
                if (errors01.size() == 0)
                {
                    maybeLoad = ParseSingleLoadExplicit(explicitLoadTable, tag);
                    if (!maybeLoad.has_value())
                    {
                        Log_Error(log, tableName, "unable to load");
                        return {};
                    }
                    if (warnings01.size() > 0)
                    {
                        for (auto const& w : warnings01)
                        {
                            Log_Warning(log, tableName, w);
                        }
                    }
                    if (maybeLoad.has_value())
                    {
                        MaybePushLoad(maybeLoad, tableName, loads);
                    }
                }
                else
                {
                    for (auto it2 = table2.cbegin(); it2 != table2.cend();
                         ++it2)
                    {
                        std::string key = it2->first;
                        toml::value value = it2->second;
                        if (key == "csv_file")
                        {
                            maybeLoad = ParseSingleLoad(
                                tag,
                                table2,
                                tableName,
                                explicitValidation,
                                fileValidation
                            );

                            if (maybeLoad.has_value())
                            {
                                MaybePushLoad(maybeLoad, tableName, loads);
                            }
                            else
                            {
                                Log_Error(
                                    log,
                                    tableName,
                                    "single load did not have value"
                                );
                                return {};
                            }
                        }
                        if (key == "multi_part_csv")
                        {
                            std::vector<std::optional<Load>> maybeLoads =
                                ParseMultiLoad(tag, table2);
                            for (auto& anotherMaybeLoad : maybeLoads)
                            {
                                if (anotherMaybeLoad.has_value())
                                {
                                    MaybePushLoad(
                                        anotherMaybeLoad, tableName, loads
                                    );
                                }
                                else
                                {
                                    Log_Error(
                                        log,
                                        tableName,
                                        "multi-part load did not have value"
                                    );
                                    return {};
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                Log_Error(log, tableName, "not a table");
                return {};
            }
        }
        return loads;
    }

    std::ostream&
    operator<<(std::ostream& os, Load const& load)
    {
        os << "Load{" << "Tag=\"" << load.Tag << "\"; " << "TimeAndLoads=[";
        for (auto it = load.TimeAndLoads.cbegin();
             it != load.TimeAndLoads.cend();
             ++it)
        {
            os << (it == load.TimeAndLoads.cbegin() ? "" : ", ") << *it;
        }
        os << "]}";
        return os;
    }

    int
    WritePackedLoads(
        const std::vector<Load>& loads,
        std::string const& loadsFilename
    )
    {
        std::ofstream out;
        out.open(
            loadsFilename, std::ofstream::out | std::ofstream::trunc
        ); // overwrite, if file exists
        if (!out.good())
        {
            std::cout << "Could not open '" << loadsFilename << "' for writing."
                      << std::endl;
            return EXIT_FAILURE;
        }

        // write header row
        bool first = true;
        for (auto it = loads.cbegin(); it != loads.cend(); ++it)
        {
            std::string tag = it->Tag;
            if (!first)
            {
                out << ",";
            }
            first = false;
            out << it->Tag << "," << it->TimeAndLoads.size();
        }
        out << "\n";

        // write units row
        first = true;
        for (auto it = loads.cbegin(); it != loads.cend(); ++it)
        {
            std::string tag = it->Tag;
            if (!first)
            {
                out << ",";
            }
            first = false;
            out << "seconds" << "," << "W";
        }
        out << "\n";

        std::size_t maxRows = 0;
        for (auto it = loads.cbegin(); it != loads.cend(); ++it)
        {
            auto nRows = it->TimeAndLoads.size();
            if (nRows > maxRows)
            {
                maxRows = nRows;
            }
        }

        // write data
        for (std::size_t iRow = 0; iRow < maxRows; ++iRow)
        {
            first = true;
            for (auto it = loads.cbegin(); it != loads.cend(); ++it)
            {
                auto& table = it->TimeAndLoads;
                if (!first)
                {
                    out << ",";
                }
                if (iRow < table.size())
                {
                    out << erin::DoubleToString(table[iRow].Time_s, 1) << ","
                        << table[iRow].Amount_W;
                }
                else
                {
                    out << ",";
                }
                first = false;
            }
            out << "\n";
        };
        out.close();
        return EXIT_SUCCESS;
    }

} // namespace erin
