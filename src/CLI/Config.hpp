#pragma once

#include <argparse/argparse.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

class Config
{
   public:
    enum class RenderMode : uint8_t
    {
        Forward,
        Bidirectional,
    };

    enum class DebugMode : uint8_t
    {
        DebugNone,
        DebugPrimaryBVHTests,
        DebugTotalBVHTests,
        DebugPrimaryTriangleTests,
        DebugTotalTriangleTests,
        DebugAlbedo,
        DebugMetallicRoughness,
        DebugGeometricNormal,
        DebugShadingNormal,
        DebugDirectOnly,
        DebugIndirectOnly,
        DebugEmission,
        DebugColorPerMesh,
        DebugColorPerTriangle,
        DebugColorPerBoundingVolume,
        DebugBounces,
        DebugDepth,
        DebugFirstHitFresnel,
        DebugFirstHitBRDF,
        DebugFirstHitPDF,
        DebugReflectedDirection,
        DebugPixelStandardDeviation,
        DebugFireflyElimination
    };

    struct ConfigOptions
    {
        std::string inputFilename;
        std::string outputFilename;

        RenderMode renderMode = RenderMode::Forward;
        DebugMode debugMode = DebugMode::DebugNone;
        unsigned imageWidth = 1280;
        unsigned imageHeight = 720;
        unsigned samples = 64;
        unsigned bounces = 5;
        unsigned cameraIndex = 0;
        unsigned sceneIndex = 0;
        float evExposure = 8.0f;

        std::string hdriOverride;
        float hdriRotation = 0.0f;
        float hdriSecondaryIntensity = 350.0f;
        float hdriPrimaryIntensity = 350.0f;
        float emissionBaseIntensity = 1.0f;

        bool bvhDisable = false;
        unsigned bvhMaxTriangles = 32;
        unsigned bvhMaxDepth = 32;
        unsigned imageBlockSize = 32;
        bool useStatisticFireflyElimination = true;
        float fireflyEliminationThreshold = 6.0f;
        float terminateEnergy = 0.01f;

        unsigned numThreads;
        bool cpuAffinity = true;

        bool quiet = false;
        bool verbose = false;
        bool veryVerbose = false;
        bool enablePreview = false;
        bool exitPreviewWhenDone = false;
    };

   private:
    ConfigOptions options_;
    argparse::ArgumentParser parser;

   private:
    Config();
    ~Config() = default;

   public:
    Config(const Config&&) = delete;
    Config& operator=(const Config&&) = delete;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

   public:
    static Config& Instance();
    void LoadConfig(int argc, char** argv);

    const ConfigOptions& GetConfigStruct() const { return options_; }
    ConfigOptions& GetConfigStructMutable() { return options_; }
    static const ConfigOptions& GetConfig() { return Instance().GetConfigStruct(); }
    static ConfigOptions& GetConfigMutable() { return Instance().GetConfigStructMutable(); }

    void LogConfig() const;
};
