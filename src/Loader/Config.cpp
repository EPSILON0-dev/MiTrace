#include "Config.hpp"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <fstream>

static constexpr char helpMessage[] =
    "Usage: RayTracing [options]\n"
    "Options:\n"
    "  -f,  --file        <file>     Specify the scene file to load.\n"
    "  -c,  --config      <config>   Specify the configuration file to load.\n"
    "  -vv, --very-verbose           Enable very verbose logging.\n"
    "  -v,  --verbose                Enable verbose logging.\n";

std::vector<std::string> Config::SplitPath(const std::string& path)
{
    std::vector<std::string> keys;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '.')) keys.push_back(item);

    return keys;
}

template <typename T>
void Config::LoadFromJson(const nlohmann::json& jsonObj, const std::string& path, T &prop)
{
    const auto* obj = &jsonObj;
    const auto keys = SplitPath(path);

    for (const auto& key : keys)
    {
        if (!obj->contains(key)) return;
        obj = &(obj->at(key));
    }

    try
    {
        prop = obj->get<T>();
    }
    catch (const std::exception& e)
    {
        spdlog::warn("Failed to read arg {}: {}", path, e.what());
    }
}

Config& Config::Instance()
{
    static Config instance;
    return instance;
}

void Config::LoadFromFile(const std::string& filepath)
{
    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse(std::ifstream(filepath));
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(
            std::format("Failed to load config file '{}': {}", filepath, e.what()));
    }

    LoadFromJson(json, "output.width", options_.image.width);
    LoadFromJson(json, "output.height", options_.image.height);
    LoadFromJson(json, "output.samples", options_.image.samples);
}

void Config::LoadConfig(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-f" || arg == "--file")
        {
            if (i + 1 < argc)
            {
                options_.input.filename = argv[++i];
            }
            else
            {
                spdlog::critical("Expected filename after {}", arg);
                exit(EXIT_FAILURE);
            }
        }
        else if (arg == "-c" || arg == "--config")
        {
            if (i + 1 < argc)
            {
                options_.input.config = argv[++i];
            }
            else
            {
                spdlog::critical("Expected filename after {}", arg);
                exit(EXIT_FAILURE);
            }
        }
        else if (arg == "-h" || arg == "--help")
        {
            printHelp_ = true;
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            verbose_ = true;
        }
        else if (arg == "-vv" || arg == "--very-verbose")
        {
            veryVerbose_ = true;
        }
        else
        {
            spdlog::critical("Unknown argument: {}", arg);
            exit(EXIT_FAILURE);
        }
    }

    if (!options_.input.config.empty()) LoadFromFile(options_.input.config);
}

bool Config::PrintHelpIfNeeded() const
{
    if (printHelp_)
    {
        std::cout << helpMessage;
        return true;
    }
    return false;
}
