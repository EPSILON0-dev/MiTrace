#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

// TODO Implement a better config loader

class Config
{
   public:
    struct ConfigOptions
    {
        struct
        {
            unsigned width = 640;
            unsigned height = 480;
            unsigned samples = 16;
        } image;

        struct
        {
            bool disableBVH = false;
            unsigned maxBounces = 5;
            unsigned maxBVHTriangles = 32;
            unsigned maxBVHDepth = 16;
            unsigned blockSize = 8;
            unsigned numThreads = 8;
            float terminateEnergy = 0.01f;
            bool cpuAffinity = true;
        } rendering;

        struct
        {
            std::string filename;
            std::string config;
        } input;
    };

   private:
    ConfigOptions options_;
    bool printHelp_ = false;
    bool verbose_ = false;
    bool enablePreview_ = false;
    bool veryVerbose_ = false;

   private:
    Config() = default;
    ~Config() = default;

   public:
    Config(const Config&&) = delete;
    Config& operator=(const Config&&) = delete;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

   private:
    static std::vector<std::string> SplitPath(const std::string& path);
    template <typename T>
    void LoadFromJson(const nlohmann::json& jsonObj, const std::string& path, T& prop);
    void LoadFromFile(const std::string& filepath);

   public:
    static Config& Instance();
    void LoadConfig(int argc, char** argv);
    bool PrintHelpIfNeeded() const;
    bool IsPreviewEnabled() const { return enablePreview_; }
    bool IsVerbose() const { return verbose_; }
    bool IsVeryVerbose() const { return veryVerbose_; }

    const ConfigOptions& GetConfigStruct() const { return options_; }
    static const ConfigOptions& GetConfig() { return Instance().GetConfigStruct(); }
};
