#include "Config.hpp"

#include <spdlog/spdlog.h>

#include <thread>

Config::Config()
{
    parser.add_argument("input")
        .help("GLTF File to be rendered (.glb is not supported yet!)")
        .store_into(options_.inputFilename)
        .required();

    parser.add_argument("-o", "--output")
        .help("Name of the file for image to be written to (.jpg, .png or .hdr)")
        .default_value(options_.outputFilename)
        .store_into(options_.outputFilename);

    parser.add_argument("-w", "--width")
        .help("Output image width in pixels")
        .default_value(options_.imageWidth)
        .store_into(options_.imageWidth);

    parser.add_argument("-h", "--height")
        .help("Output image height in pixels")
        .default_value(options_.imageHeight)
        .store_into(options_.imageHeight);

    parser.add_argument("-s", "--samples")
        .help("Samples per pixel")
        .default_value(options_.samples)
        .store_into(options_.samples);

    parser.add_argument("-b", "--bounces")
        .help("Maximum path bounces")
        .default_value(options_.bounces)
        .store_into(options_.bounces);

    parser.add_argument("-e", "--ev-exposure")
        .help("Exposure value applied during tonemapping")
        .default_value(std::to_string(options_.evExposure));

    parser.add_argument("--disable-bvh")
        .help("Disable BVH acceleration structure")
        .default_value(options_.bvhDisable)
        .implicit_value(true)
        .store_into(options_.bvhDisable);

    parser.add_argument("--bvh-max-triangles")
        .help("Maximum number of triangles per BVH leaf")
        .default_value(options_.bvhMaxTriangles)
        .store_into(options_.bvhMaxTriangles);

    parser.add_argument("--bvh-max-depth")
        .help("Maximum BVH recursion depth")
        .default_value(options_.bvhMaxDepth)
        .store_into(options_.bvhMaxDepth);

    parser.add_argument("--image-block-size")
        .help("Tile size used for rendering work distribution")
        .default_value(options_.imageBlockSize)
        .store_into(options_.imageBlockSize);

    parser.add_argument("--disable-firefly-elimination")
        .help("Disable statistical firefly elimination")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("--firefly-threshold")
        .help("Threshold used by firefly elimination (in standard deviations)")
        .default_value(std::to_string(options_.fireflyEliminationThreshold));

    parser.add_argument("--terminate-energy")
        .help("Path termination energy threshold")
        .default_value(std::to_string(options_.terminateEnergy));

    parser.add_argument("-t", "--threads")
        .help("Number of worker threads")
        .default_value(std::thread::hardware_concurrency())
        .store_into(options_.numThreads);

    parser.add_argument("--disable-cpu-affinity")
        .help("Disable CPU affinity pinning for worker threads")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("-v", "--verbose")
        .help("Enable verbose logging")
        .default_value(options_.verbose)
        .implicit_value(true)
        .store_into(options_.verbose);

    parser.add_argument("-p", "--preview")
        .help("Enable preview window")
        .default_value(options_.enablePreview)
        .implicit_value(true)
        .store_into(options_.enablePreview);

    parser.add_argument("-V", "--very-verbose")
        .help("Enable very verbose logging")
        .default_value(options_.veryVerbose)
        .implicit_value(true)
        .store_into(options_.veryVerbose);
}

Config& Config::Instance()
{
    static Config instance;
    return instance;
}

void Config::LoadConfig(int argc, char** argv)
{
    parser.parse_args(argc, argv);

    options_.evExposure = std::stof(parser.get<std::string>("--ev-exposure"));
    options_.fireflyEliminationThreshold =
        std::stof(parser.get<std::string>("--firefly-threshold"));
    options_.terminateEnergy = std::stof(parser.get<std::string>("--terminate-energy"));
    options_.useStatisticFireflyElimination = !parser.get<bool>("--disable-firefly-elimination");
    options_.cpuAffinity = !parser.get<bool>("--disable-cpu-affinity");
}

void Config::LogConfig() const
{
    const auto& o = options_;
    spdlog::debug("Config:");
    spdlog::debug("  inputFilename: {}", o.inputFilename);
    spdlog::debug("  outputFilename: {}", o.outputFilename);
    spdlog::debug("  imageWidth: {}", o.imageWidth);
    spdlog::debug("  imageHeight: {}", o.imageHeight);
    spdlog::debug("  samples: {}", o.samples);
    spdlog::debug("  bounces: {}", o.bounces);
    spdlog::debug("  evExposure: {}", o.evExposure);
    spdlog::debug("  bvhDisable: {}", o.bvhDisable ? "true" : "false");
    spdlog::debug("  bvhMaxTriangles: {}", o.bvhMaxTriangles);
    spdlog::debug("  bvhMaxDepth: {}", o.bvhMaxDepth);
    spdlog::debug("  imageBlockSize: {}", o.imageBlockSize);
    spdlog::debug("  useStatisticFireflyElimination: {}",
        o.useStatisticFireflyElimination ? "true" : "false");
    spdlog::debug("  fireflyEliminationThreshold: {}", o.fireflyEliminationThreshold);
    spdlog::debug("  terminateEnergy: {}", o.terminateEnergy);
    spdlog::debug("  numThreads: {}", o.numThreads);
    spdlog::debug("  cpuAffinity: {}", o.cpuAffinity ? "true" : "false");
    spdlog::debug("  verbose: {}", o.verbose ? "true" : "false");
    spdlog::debug("  enablePreview: {}", o.enablePreview ? "true" : "false");
    spdlog::debug("  veryVerbose: {}", o.veryVerbose ? "true" : "false");
}
