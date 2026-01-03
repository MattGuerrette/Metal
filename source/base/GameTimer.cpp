////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024. Matt Guerrette
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "GameTimer.hpp"

GameTimer::GameTimer()
    : m_qpcSecondCounter(0)
    , m_elapsedTicks(0)
    , m_totalTicks(0)
    , m_leftOverTicks(0)
    , m_frameCount(0)
    , m_framesPerSecond(0)
    , m_framesThisSecond(0)
    , m_isFixedTimeStep(false)
    , m_targetElapsedTicks(0)
{
    m_qpcFrequency = SDL_GetPerformanceFrequency();
    m_qpcLastTime = SDL_GetPerformanceCounter();

    // Max delta 1/10th of a second
    m_qpcMaxDelta = m_qpcFrequency / 10;
}

uint64_t GameTimer::elapsedTicks() const noexcept
{
    return m_elapsedTicks;
}

double GameTimer::elapsedSeconds() const noexcept
{
    return ticksToSeconds(m_elapsedTicks);
}

uint64_t GameTimer::totalTicks() const noexcept
{
    return m_totalTicks;
}

double GameTimer::totalSeconds() const noexcept
{
    return ticksToSeconds(m_totalTicks);
}

uint32_t GameTimer::frameCount() const noexcept
{
    return m_frameCount;
}

uint32_t GameTimer::framesPerSecond() const noexcept
{
    return m_framesPerSecond;
}

void GameTimer::setFixedTimeStep(const bool isFixedTimeStep) noexcept
{
    m_isFixedTimeStep = isFixedTimeStep;
}

void GameTimer::setTargetElapsedTicks(uint64_t targetElapsed) noexcept
{
    m_targetElapsedTicks = targetElapsed;
}

void GameTimer::setTargetElapsedSeconds(const double targetElapsed) noexcept
{
    m_targetElapsedTicks = secondsToTicks(targetElapsed);
}

void GameTimer::resetElapsedTime()
{
    m_qpcLastTime = SDL_GetPerformanceCounter();

    m_leftOverTicks = 0;
    m_framesPerSecond = 0;
    m_framesThisSecond = 0;
    m_qpcSecondCounter = 0;
    m_totalTicks = 0;
}
