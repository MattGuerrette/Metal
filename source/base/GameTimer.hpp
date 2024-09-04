////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <SDL3/SDL.h>

#include <algorithm>

class GameTimer {
    static constexpr uint64_t s_ticksPerSecond = 10000000;

public:
    GameTimer();

    ~GameTimer() = default;

    [[nodiscard]] uint64_t elapsedTicks() const noexcept;

    [[nodiscard]] double elapsedSeconds() const noexcept;

    [[nodiscard]] uint64_t totalTicks() const noexcept;

    [[nodiscard]] double totalSeconds() const noexcept;

    [[nodiscard]] uint32_t frameCount() const noexcept;

    [[nodiscard]] uint32_t framesPerSecond() const noexcept;

    void setFixedTimeStep(bool isFixedTimeStep) noexcept;

    void setTargetElapsedTicks(uint64_t targetElapsed) noexcept;

    void setTargetElapsedSeconds(double targetElapsed) noexcept;

    void resetElapsedTime();

    template <typename TUpdate>
    void tick(const TUpdate& update)
    {
        const uint64_t currentTime = SDL_GetPerformanceCounter();

        uint64_t delta = currentTime - m_qpcLastTime;
        m_qpcLastTime = currentTime;
        m_qpcSecondCounter += delta;

        delta = std::clamp(delta, static_cast<uint64_t>(0), m_qpcMaxDelta);
        delta *= s_ticksPerSecond;
        delta /= m_qpcFrequency;

        const uint32_t lastFrameCount = m_frameCount;
        if (m_isFixedTimeStep) {
            if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(delta - m_targetElapsedTicks)))
                < s_ticksPerSecond / 4000) {
                delta = m_targetElapsedTicks;
            }

            m_leftOverTicks += delta;
            while (m_leftOverTicks >= m_targetElapsedTicks) {
                m_elapsedTicks = m_targetElapsedTicks;
                m_totalTicks += m_targetElapsedTicks;
                m_leftOverTicks -= m_targetElapsedTicks;
                m_frameCount++;

                update();
            }
        } else {
            m_elapsedTicks = delta;
            m_totalTicks += delta;
            m_leftOverTicks = 0;
            m_frameCount++;

            update();
        }

        if (m_frameCount != lastFrameCount) {
            m_framesThisSecond++;
        }

        if (m_qpcSecondCounter >= m_qpcFrequency) {
            m_framesPerSecond = m_framesThisSecond;
            m_framesThisSecond = 0;
            m_qpcSecondCounter %= m_qpcFrequency;
        }
    }

    static constexpr double ticksToSeconds(const uint64_t ticks) noexcept
    {
        return static_cast<double>(ticks) / s_ticksPerSecond;
    }

    static constexpr uint64_t secondsToTicks(const double seconds) noexcept
    {
        return static_cast<uint64_t>(seconds * s_ticksPerSecond);
    }

private:
    uint64_t m_qpcFrequency;
    uint64_t m_qpcLastTime;
    uint64_t m_qpcMaxDelta;
    uint64_t m_qpcSecondCounter;
    uint64_t m_elapsedTicks;
    uint64_t m_totalTicks;
    uint64_t m_leftOverTicks;
    uint32_t m_frameCount;
    uint32_t m_framesPerSecond;
    uint32_t m_framesThisSecond;
    bool     m_isFixedTimeStep;
    uint64_t m_targetElapsedTicks;
};
