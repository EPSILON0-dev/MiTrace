#include "Config.hpp"

#include <spdlog/spdlog.h>

#include <iostream>

#include "Common/Logger.pch.hpp"  // IWYU pragma: keep

static constexpr char helpMessage[] =
    "Usage: {} [options]\n"
    "Options:\n"
    "  -f, --file        <file>     Specify the configuration file to load.\n"
    "  -w, --width       <pixels>   Set the image width in pixels.\n"
    "  -h, --height      <pixels>   Set the image height in pixels.\n"
    "  -s, --samples     <number>   Set the number of samples per pixel.\n"
    "  -b, --bounces     <number>   Set the maximum number of bounces per ray.\n"
    "  -B, --block-size <pixels>    Set the size of each render block in pixels.\n"
    "  -m, --mode        <mode>     Set the render mode (Render, FresnelPreview).\n"
    "  -h, --help                   Display this help message.\n";

Config& Config::Instance()
{
    static Config instance;
    return instance;
}

void Config::LoadFromFile(const char* filepath)
{
    // Implementation for loading configuration from a file
    SPDLOG_INFO("Loading from a file currently not implemented.");
    (void)filepath;
}

void Config::LoadFromArgs(int argc, char** argv)
{
    binaryName_ = argv[0];

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-f" || arg == "--file")
        {
            if (i + 1 < argc)
            {
                inputFilePath_ = argv[++i];
            }
        }
        else if (arg == "-w" || arg == "--width")
        {
            if (i + 1 < argc)
            {
                imageWidth_ = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
        }
        else if (arg == "-h" || arg == "--height")
        {
            if (i + 1 < argc)
            {
                imageHeight_ = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
        }
        else if (arg == "-s" || arg == "--samples")
        {
            if (i + 1 < argc)
            {
                samplesPerPixel_ = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
        }
        else if (arg == "-b" || arg == "--bounces")
        {
            if (i + 1 < argc)
            {
                maxBounces_ = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
        }
        else if (arg == "-S" || arg == "--sector-size")
        {
            if (i + 1 < argc)
            {
                sectorSize_ = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
        }
        else if (arg == "-t" || arg == "--threads")
        {
            if (i + 1 < argc)
            {
                threadCount_ = static_cast<uint32_t>(std::stoi(argv[++i]));
            }
        }
        else if (arg == "-m" || arg == "--mode")
        {
            if (i + 1 < argc)
            {
                std::string modeStr = argv[++i];
                if (modeStr == "Render")
                    renderMode_ = RenderMode::Render;
                else if (modeStr == "FresnelPreview")
                    renderMode_ = RenderMode::FresnelPreview;
            }
        }
        else if (arg == "-h" || arg == "--help")
        {
            printHelp_ = true;
        }
        else
        {
            SPDLOG_CRITICAL("Unknown argument: {}", arg);
            exit(EXIT_FAILURE);
        }
    }
}

bool Config::PrintHelpIfNeeded() const
{
    if (printHelp_)
    {
        std::cout << std::format(helpMessage, binaryName_);
        return true;
    }
    return false;
}
