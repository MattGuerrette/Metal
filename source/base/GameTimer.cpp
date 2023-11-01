#include "GameTimer.hpp"


GameTimer::GameTimer()
	: m_elapsedTicks(0), m_totalTicks(0), m_leftOverTicks(0), m_frameCount(0), m_framesPerSecond(0),
	  m_framesThisSecond(0),
	  m_qpcSecondCounter(0),
	  m_isFixedTimeStep(false),
	  m_targetElapsedTicks(0)
{
	m_qpcFrequency = SDL_GetPerformanceFrequency();
	m_qpcLastTime = SDL_GetPerformanceCounter();


	// Max delta 1/10th of a second
	m_qpcMaxDelta = m_qpcFrequency / 10;
}

uint64_t GameTimer::GetElapsedTicks() const noexcept
{
	return m_elapsedTicks;
}

double GameTimer::GetElapsedSeconds() const noexcept
{
	return TicksToSeconds(m_elapsedTicks);
}

uint64_t GameTimer::GetTotalTicks() const noexcept
{
	return m_totalTicks;
}

double GameTimer::GetTotalSeconds() const noexcept
{
	return TicksToSeconds(m_totalTicks);
}

uint32_t GameTimer::GetFrameCount() const noexcept
{
	return m_frameCount;
}

uint32_t GameTimer::GetFramesPerSecond() const noexcept
{
	return m_framesPerSecond;
}

void GameTimer::SetFixedTimeStep(const bool isFixedTimeStep) noexcept
{
	m_isFixedTimeStep = isFixedTimeStep;
}

void GameTimer::SetTargetElapsedTicks(const uint64_t targetElapsed) noexcept
{
	m_targetElapsedTicks = targetElapsed;
}

void GameTimer::SetTargetElapsedSeconds(const double targetElapsed) noexcept
{
	m_targetElapsedTicks = SecondsToTicks(targetElapsed);
}

void GameTimer::ResetElapsedTime()
{
	m_qpcLastTime = SDL_GetPerformanceCounter();

	m_leftOverTicks = 0;
	m_framesPerSecond = 0;
	m_framesThisSecond = 0;
	m_qpcSecondCounter = 0;
	m_totalTicks = 0;
}
