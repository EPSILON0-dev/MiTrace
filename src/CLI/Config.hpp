#pragma once

#include <argparse/argparse.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

// TODO Implement a better config loader

class Config
{
   public:
    struct ConfigOptions
    {
        std::string inputFilename;
        std::string outputFilename;

        unsigned imageWidth = 1280;
        unsigned imageHeight = 720;
        unsigned samples = 64;
        unsigned bounces = 5;
        float evExposure = 8.0f;

        bool bvhDisable = false;
        unsigned bvhMaxTriangles = 32;
        unsigned bvhMaxDepth = 32;
        unsigned imageBlockSize = 32;
        bool useStatisticFireflyElimination = true;
        float fireflyEliminationThreshold = 6.0f;
        float terminateEnergy = 0.01f;

        unsigned numThreads;
        bool cpuAffinity = true;

        bool verbose = false;
        bool enablePreview = false;
        bool veryVerbose = false;
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
    static const ConfigOptions& GetConfig() { return Instance().GetConfigStruct(); }

    void LogConfig() const;
};
