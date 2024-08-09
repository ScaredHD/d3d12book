#include "Timer.h"

#undef max
#include <algorithm>

Timer::Timer() : isRunning{false} {
    TimeCount countsPerSeconds;
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSeconds));
    secondsPerCount = 1.0 / static_cast<double>(countsPerSeconds);
}

float Timer::TotalTimeFromStart() const {
    TimeCount interval = isRunning ? (GetCurrentCount() - baseCount) - totalPausedCount
                                   : interval = (stopCount - baseCount) - totalPausedCount;
    return static_cast<float>(interval * secondsPerCount);
}

void Timer::Reset() {
    baseCount = GetCurrentCount();
    prevCount = baseCount;
    stopCount = 0;
    isRunning = true;
}

void Timer::Start() {
    if (isRunning) {
        return;
    }

    isRunning = true;
    auto t = GetCurrentCount();
    totalPausedCount += t - stopCount;
    prevCount = t;
}

void Timer::Stop() {
    if (!isRunning) {
        return;
    }

    isRunning = false;
    stopCount = GetCurrentCount();
}

void Timer::Tick() {
    if (!isRunning) {
        deltaTime = 0.0;
        return;
    }

    currCount = GetCurrentCount();

    deltaTime = static_cast<double>(currCount - prevCount) * secondsPerCount;
    deltaTime = std::max(deltaTime, 0.0);

    prevCount = currCount;
}

Timer::TimeCount Timer::GetCurrentCount() {
    TimeCount t;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&t));
    return t;
}