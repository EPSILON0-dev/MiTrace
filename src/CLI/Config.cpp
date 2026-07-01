#include "Config.hpp"

#include <spdlog/spdlog.h>

#include <thread>
#include <utility>
#include <vector>

#define VERSION "MiTrace " MITRACE_VERSION " compiled at " __DATE__ " " __TIME__

const static std::vector<std::pair<std::string, Config::DebugMode>> debugModeMap = {
    std::make_pair("none", Config::DebugMode::DebugNone),
    std::make_pair("debug-primary-bvh-tests", Config::DebugMode::DebugPrimaryBVHTests),
    std::make_pair("debug-total-bvh-tests", Config::DebugMode::DebugTotalBVHTests),
    std::make_pair("debug-primary-triangle-tests", Config::DebugMode::DebugPrimaryTriangleTests),
    std::make_pair("debug-total-triangle-tests", Config::DebugMode::DebugTotalTriangleTests),
    std::make_pair("debug-albedo", Config::DebugMode::DebugAlbedo),
    std::make_pair("debug-metallic-roughness", Config::DebugMode::DebugMetallicRoughness),
    std::make_pair("debug-geometric-normal", Config::DebugMode::DebugGeometricNormal),
    std::make_pair("debug-shading-normal", Config::DebugMode::DebugShadingNormal),
    std::make_pair("debug-direct-only", Config::DebugMode::DebugDirectOnly),
    std::make_pair("debug-indirect-only", Config::DebugMode::DebugIndirectOnly),
    std::make_pair("debug-emission", Config::DebugMode::DebugEmission),
    std::make_pair("debug-color-per-mesh", Config::DebugMode::DebugColorPerMesh),
    std::make_pair("debug-color-per-triangle", Config::DebugMode::DebugColorPerTriangle),
    std::make_pair(
        "debug-color-per-bounding-volume", Config::DebugMode::DebugColorPerBoundingVolume),
    std::make_pair("debug-bounces", Config::DebugMode::DebugBounces),
    std::make_pair("debug-depth", Config::DebugMode::DebugDepth),
    std::make_pair("debug-first-hit-fresnel", Config::DebugMode::DebugFirstHitFresnel),
    std::make_pair("debug-first-hit-brdf", Config::DebugMode::DebugFirstHitBRDF),
    std::make_pair("debug-first-hit-pdf", Config::DebugMode::DebugFirstHitPDF),
    std::make_pair("debug-reflected-direction", Config::DebugMode::DebugReflectedDirection),
    std::make_pair(
        "debug-pixel-standard-deviation", Config::DebugMode::DebugPixelStandardDeviation),
    std::make_pair(
        "debug-firefly-elimination", Config::DebugMode::DebugFireflyElimination)};

Config::Config() : parser("MiTrace", VERSION)
{
    options_ = ConfigOptions();
    options_.renderMode = Config::RenderMode::Forward;
    options_.debugMode = Config::DebugMode::DebugNone;

    parser.add_argument("input")
        .help("GLTF File to be rendered (.glb is not supported yet!)")
        .store_into(options_.inputFilename)
        .required();

    parser.add_argument("-o", "--output")
        .help("Name of the file for image to be written to (.jpg, .png or .hdr)")
        .default_value(options_.outputFilename)
        .store_into(options_.outputFilename);

    parser.add_argument("-m", "--mode")
        .help("Render mode: forward, bidirectional, debug")
        .default_value("forward")
        .action(
            [this](const std::string& value)
            {
                if (value == "forward")
                    options_.renderMode = Config::RenderMode::Forward;
                else if (value == "bidirectional")
                    options_.renderMode = Config::RenderMode::Bidirectional;
                else
                    throw std::runtime_error("Invalid render mode: " + value);
            });

    parser.add_argument("--debug-mode")
        .help("Debug mode: try --list-debug-modes to see all available debug modes. ")
        .default_value("none")
        .action(
            [this](const std::string& value)
            {
                const auto& pair = std::ranges::find_if(
                    debugModeMap, [&](const auto& p) { return p.first == value; });
                if (pair == debugModeMap.end())
                    throw std::runtime_error("Invalid debug mode: " + value);
                options_.debugMode = pair->second;
            });

    parser.add_argument("--list-debug-modes")
        .help("List all available debug modes")
        .implicit_value(true)
        .default_value(false);

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

    parser.add_argument("-c", "--camera")
        .help("Camera index to use for rendering")
        .default_value(options_.cameraIndex)
        .store_into(options_.cameraIndex);

    parser.add_argument("--scene")
        .help("Scene index to use for rendering")
        .default_value(options_.sceneIndex)
        .store_into(options_.sceneIndex);

    parser.add_argument("-e", "--ev-exposure")
        .help("Exposure value applied during tonemapping")
        .default_value(std::to_string(options_.evExposure));

    parser.add_argument("-hdri", "--hdri-override")
        .help("HDRI file to use for environment lighting (must be .hdr or .png)")
        .store_into(options_.hdriOverride);

    parser.add_argument("--hdri-rotation")
        .help("HDRI rotation in degrees")
        .default_value(std::to_string(options_.hdriRotation));

    parser.add_argument("--hdri-secondary-intensity")
        .help("HDRI secondary intensity")
        .default_value(std::to_string(options_.hdriSecondaryIntensity));

    parser.add_argument("--hdri-primary-intensity")
        .help("HDRI primary intensity")
        .default_value(std::to_string(options_.hdriPrimaryIntensity));

    parser.add_argument("-e", "--ev-exposure")
        .help("Exposure value applied during tonemapping")
        .default_value(std::to_string(options_.evExposure));

    parser.add_argument("--emission-base-intensity")
        .help("Base intensity for emissive materials")
        .default_value(std::to_string(options_.emissionBaseIntensity));

    parser.add_argument("--disable-bvh")
        .help("Disable BVH acceleration structure")
        .default_value(false)
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
        .default_value(true)
        .implicit_value(false)
        .store_into(options_.useStatisticFireflyElimination);

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
        .default_value(false)
        .implicit_value(true)
        .store_into(options_.verbose);

    parser.add_argument("-p", "--preview")
        .help("Enable preview window")
        .default_value(false)
        .implicit_value(true)
        .store_into(options_.enablePreview);

    parser.add_argument("-x", "--exit-preview-when-done")
        .help("Close the preview window when render is completed")
        .default_value(false)
        .implicit_value(true)
        .store_into(options_.exitPreviewWhenDone);

    parser.add_argument("-V", "--very-verbose")
        .help("Enable very verbose logging")
        .default_value(false)
        .implicit_value(true)
        .store_into(options_.veryVerbose);

    parser.add_argument("-q", "--quiet")
        .help("Disable everything except for errors and progress bar")
        .default_value(false)
        .implicit_value(true)
        .store_into(options_.quiet);
}

Config& Config::Instance()
{
    static Config instance;
    return instance;
}

void Config::LoadConfig(int argc, char** argv)
{
    static const char* helpArgs[] = {"MiTrace", "--help"};
    if (argc == 1)
        parser.parse_args(2, helpArgs);
    else
        parser.parse_args(argc, argv);

    options_.evExposure = std::stof(parser.get<std::string>("--ev-exposure"));
    options_.fireflyEliminationThreshold =
        std::stof(parser.get<std::string>("--firefly-threshold"));
    options_.terminateEnergy = std::stof(parser.get<std::string>("--terminate-energy"));
    options_.useStatisticFireflyElimination = !parser.get<bool>("--disable-firefly-elimination");
    options_.hdriRotation = std::stof(parser.get<std::string>("--hdri-rotation"));
    options_.hdriSecondaryIntensity =
        std::stof(parser.get<std::string>("--hdri-secondary-intensity"));
    options_.hdriPrimaryIntensity = std::stof(parser.get<std::string>("--hdri-primary-intensity"));
    options_.emissionBaseIntensity =
        std::stof(parser.get<std::string>("--emission-base-intensity"));
    options_.cpuAffinity = !parser.get<bool>("--disable-cpu-affinity");

    if (parser.get<bool>("--list-debug-modes"))
    {
        spdlog::info("Available debug modes:");
        for (const auto& pair : debugModeMap)
        {
            spdlog::info("  {}", pair.first);
        }
        std::exit(EXIT_SUCCESS);
    }
}

void Config::LogConfig() const
{
    const auto& o = options_;

    std::string renderModeStr = (o.renderMode == RenderMode::Forward) ? "forward" : "bidirectional";
    std::string debugModeStr = std::ranges::find_if(debugModeMap,
        [&](const auto& pair)
        {
            return pair.second == o.debugMode;
        })->first;

    spdlog::debug("Config:");
    spdlog::debug("  mode: {}", renderModeStr);
    spdlog::debug("  debugMode: {}", debugModeStr);
    spdlog::debug("  inputFilename: {}", o.inputFilename);
    spdlog::debug("  outputFilename: {}", o.outputFilename);
    spdlog::debug("  imageWidth: {}", o.imageWidth);
    spdlog::debug("  imageHeight: {}", o.imageHeight);
    spdlog::debug("  samples: {}", o.samples);
    spdlog::debug("  bounces: {}", o.bounces);
    spdlog::debug("  cameraIndex: {}", o.cameraIndex);
    spdlog::debug("  sceneIndex: {}", o.sceneIndex);
    spdlog::debug("  evExposure: {}", o.evExposure);
    spdlog::debug("  hdriOverride: {}", o.hdriOverride.empty() ? "None" : o.hdriOverride);
    spdlog::debug("  hdriRotation: {}", o.hdriRotation);
    spdlog::debug("  hdriSecondaryIntensity: {}", o.hdriSecondaryIntensity);
    spdlog::debug("  hdriPrimaryIntensity: {}", o.hdriPrimaryIntensity);
    spdlog::debug("  emissionBaseIntensity: {}", o.emissionBaseIntensity);
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
    spdlog::debug("  exitPreviewWhenDone: {}", o.exitPreviewWhenDone ? "true" : "false");
    spdlog::debug("  veryVerbose: {}", o.veryVerbose ? "true" : "false");
}
