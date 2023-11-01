#include "GameTimer.hpp"


GameTimer::GameTimer()
	: ElapsedTicks(0), TotalTicks(0), LeftOverTicks(0), FrameCount(0), FramesPerSecond(0),
	  FramesThisSecond(0),
	  QpcSecondCounter(0),
	  IsFixedTimeStep(false),
	  TargetElapsedTicks(0)
{
	QpcFrequency = SDL_GetPerformanceFrequency();
	QpcLastTime = SDL_GetPerformanceCounter();


	// Max delta 1/10th of a second
	QpcMaxDelta = QpcFrequency / 10;
}

uint64_t GameTimer::GetElapsedTicks() const noexcept
{
	return ElapsedTicks;
}

double GameTimer::GetElapsedSeconds() const noexcept
{
	return TicksToSeconds(ElapsedTicks);
}

uint64_t GameTimer::GetTotalTicks() const noexcept
{
	return TotalTicks;
}

double GameTimer::GetTotalSeconds() const noexcept
{
	return TicksToSeconds(TotalTicks);
}

uint32_t GameTimer::GetFrameCount() const noexcept
{
	return FrameCount;
}

uint32_t GameTimer::GetFramesPerSecond() const noexcept
{
	return FramesPerSecond;
}

void GameTimer::SetFixedTimeStep(const bool isFixedTimeStep) noexcept
{
	IsFixedTimeStep = isFixedTimeStep;
}

void GameTimer::SetTargetElapsedTicks(const uint64_t targetElapsed) noexcept
{
	TargetElapsedTicks = targetElapsed;
}

void GameTimer::SetTargetElapsedSeconds(const double targetElapsed) noexcept
{
	TargetElapsedTicks = SecondsToTicks(targetElapsed);
}

void GameTimer::ResetElapsedTime()
{
	QpcLastTime = SDL_GetPerformanceCounter();

	LeftOverTicks = 0;
	FramesPerSecond = 0;
	FramesThisSecond = 0;
	QpcSecondCounter = 0;
	TotalTicks = 0;
}
