#pragma once

#include <windows.h>

class MyTimer {
  public:
    using TimeCount = __int64;

    static TimeCount GetCurrentCount();

    MyTimer();

    float TotalTimeFromStart() const;

    float DeltaTimeSecond() const { return deltaTime; }

    void Reset();

    void Start();

    void Stop();

    void Tick();

  private:
    bool isRunning;

    double secondsPerCount = 0.0;
    double deltaTime = -1.0;

    // Keep track of time elapsed between frames
    TimeCount prevCount = 0;
    TimeCount currCount = 0;

    // Keep track of total time since start (excluding pauses)
    TimeCount baseCount = 0;
    TimeCount totalPausedCount = 0;
    TimeCount stopCount = 0;
};
