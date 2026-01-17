#pragma once

#include <cstdint>
#include <string>

class Config
{
   public:
    enum class RenderMode
    {
        Render,
        FresnelPreview,
    };

   private:
    std::string inputFilePath_;
    std::string binaryName_;
    uint32_t imageWidth_ = 800;
    uint32_t imageHeight_ = 600;
    uint32_t samplesPerPixel_ = 8;
    uint32_t maxBounces_ = 5;
    std::optional<uint32_t> threadCount_ = std::nullopt;
    uint32_t sectorSize_ = 64;
    bool printHelp_ = false;
    RenderMode renderMode_ = RenderMode::Render;

   private:
    Config() = default;
    ~Config() = default;
    Config(const Config&&) = delete;
    Config& operator=(const Config&&) = delete;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

   public:
    static Config& Instance();
    void LoadFromFile(const char* filepath);
    void LoadFromArgs(int argc, char** argv);
    bool PrintHelpIfNeeded() const;

    const std::string& InputFilePath() const { return inputFilePath_; }
    uint32_t ImageWidth() const { return imageWidth_; }
    uint32_t ImageHeight() const { return imageHeight_; }
    uint32_t SamplesPerPixel() const { return samplesPerPixel_; }
    uint32_t MaxBounces() const { return maxBounces_; }
    uint32_t SectorSize() const { return sectorSize_; }
    std::optional<uint32_t> GetThreadCount() const { return threadCount_; }
    RenderMode GetRenderMode() const { return renderMode_; }
};
