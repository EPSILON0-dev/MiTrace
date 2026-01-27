#pragma once

#include <string>
#include <nlohmann/json_fwd.hpp>

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
            std::string filename = "";
            std::string config = "";
        } input;

        struct
        {
            unsigned bounces = 5;
            unsigned pixelsPerBucket = 16;
            unsigned samplesPerBucket = 8;
        } render;
    };

   private:
    ConfigOptions options_;
    bool printHelp_ = false;
    bool verbose_ = false;
    bool veryVerbose_ = false;

   private:
    Config() = default;
    ~Config() = default;
    Config(const Config&&) = delete;
    Config& operator=(const Config&&) = delete;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

   private:
    static std::vector<std::string> SplitPath(const std::string& path);
    template <typename T>
    void LoadFromJson(const nlohmann::json& jsonObj, const std::string& path, T &prop);
    void LoadFromFile(const std::string& filepath);

   public:
    static Config& Instance();
    void LoadConfig(int argc, char** argv);
    bool PrintHelpIfNeeded() const;
    bool IsVerbose() const { return verbose_; }
    bool IsVeryVerbose() const { return veryVerbose_; }

    const ConfigOptions& GetConfig() const { return options_; }
};
