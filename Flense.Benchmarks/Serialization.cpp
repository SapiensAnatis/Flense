#include "pch.h"

#include "Serialization.h"

#include <nlohmann/json.hpp>

namespace Flense::Benchmarks
{
    namespace
    {
        nlohmann::json ToJson(const PhaseResult& phase)
        {
            return {
                {"phase", PhaseName(phase.phase)},
                {"samplesMs", phase.samplesMs},
            };
        }

        nlohmann::json ToJson(const EntryTiming& entry)
        {
            return {
                {"pathname", entry.pathname},
                {"size", entry.size},
                {"milliseconds", entry.milliseconds},
            };
        }

        nlohmann::json ToJson(const Counters& counters)
        {
            return {
                {"entryCount", counters.entryCount},
                {"layerCount", counters.layerCount},
                {"treeNodeCount", counters.treeNodeCount},
                {"uniqueTreeNodeCount", counters.uniqueTreeNodeCount},
                {"peakWorkingSetBytes", counters.peakWorkingSetBytes},
            };
        }
    } // namespace

    std::string SerializeResult(const BenchmarkResult& result)
    {
        nlohmann::json phases = nlohmann::json::array();
        for (const PhaseResult& phase : result.phases)
        {
            phases.push_back(ToJson(phase));
        }

        nlohmann::json slowestEntries = nlohmann::json::array();
        for (const EntryTiming& entry : result.slowestEntries)
        {
            slowestEntries.push_back(ToJson(entry));
        }

        const nlohmann::json root{
            {"imagePath", result.imagePath.string()},
            {"imageSizeBytes", result.imageSizeBytes},
            {"runs", result.runs},
            {"phases", phases},
            {"counters", ToJson(result.counters)},
            {"slowestEntries", slowestEntries},
        };

        return root.dump();
    }

    BenchmarkResult DeserializeResult(const std::string_view json)
    {
        const nlohmann::json root = nlohmann::json::parse(json);

        BenchmarkResult result;
        result.imagePath = root.at("imagePath").get<std::string>();
        result.imageSizeBytes = root.at("imageSizeBytes").get<uint64_t>();
        result.runs = root.at("runs").get<int>();

        for (const auto& phaseJson : root.at("phases"))
        {
            result.phases.push_back(PhaseResult{
                .phase = ParsePhaseName(phaseJson.at("phase").get<std::string>()),
                .samplesMs = phaseJson.at("samplesMs").get<std::vector<double>>(),
            });
        }

        const nlohmann::json& countersJson = root.at("counters");
        result.counters = Counters{
            .entryCount = countersJson.at("entryCount").get<size_t>(),
            .layerCount = countersJson.at("layerCount").get<size_t>(),
            .treeNodeCount = countersJson.at("treeNodeCount").get<size_t>(),
            .uniqueTreeNodeCount = countersJson.at("uniqueTreeNodeCount").get<size_t>(),
            .peakWorkingSetBytes = countersJson.at("peakWorkingSetBytes").get<uint64_t>(),
        };

        for (const auto& entryJson : root.at("slowestEntries"))
        {
            result.slowestEntries.push_back(EntryTiming{
                .pathname = entryJson.at("pathname").get<std::string>(),
                .size = entryJson.at("size").get<uint64_t>(),
                .milliseconds = entryJson.at("milliseconds").get<double>(),
            });
        }

        return result;
    }
} // namespace Flense::Benchmarks
