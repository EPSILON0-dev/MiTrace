#pragma once

class Tracer
{
   public:
    struct Stats
    {
        float progress;
        float timeElapsed;
        float estimatedTimeRemaining;
        unsigned raysTraced;
    };

   public:
    Tracer() = default;
    virtual ~Tracer() = default;

    // Starts the rendering process
    virtual void StartRender() = 0;

    // Waits until the rendering process is complete
    virtual void WaitForRender() = 0;

    // Forcefully kills the rendering process if it's still running
    virtual void KillRender() = 0;

    // Retrieves the current rendering statistics
    virtual Stats GetStats() const = 0;

    // Checks if the rendering process is complete
    virtual bool IsDone() const = 0;
};
