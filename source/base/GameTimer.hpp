#pragma once

#include <SDL.h>

#include <algorithm>


class GameTimer
{
	static constexpr uint64_t TicksPerSecond = 10000000;

public:
	GameTimer();

	~GameTimer() = default;

	[[nodiscard]] uint64_t GetElapsedTicks() const noexcept;

	[[nodiscard]] double GetElapsedSeconds() const noexcept;

	[[nodiscard]] uint64_t GetTotalTicks() const noexcept;

	[[nodiscard]] double GetTotalSeconds() const noexcept;

	[[nodiscard]] uint32_t GetFrameCount() const noexcept;

	[[nodiscard]] uint32_t GetFramesPerSecond() const noexcept;

	void SetFixedTimeStep(bool isFixedTimeStep) noexcept;

	void SetTargetElapsedTicks(uint64_t targetElapsed) noexcept;

	void SetTargetElapsedSeconds(double targetElapsed) noexcept;

	void ResetElapsedTime();

	template<typename TUpdate>
	void Tick(const TUpdate& update)
	{
		const uint64_t currentTime = SDL_GetPerformanceCounter();

		uint64_t delta = currentTime - QpcLastTime;
		QpcLastTime = currentTime;
		QpcSecondCounter += delta;

		delta = std::clamp(delta, static_cast<uint64_t>(0), QpcMaxDelta);
		delta *= TicksPerSecond;
		delta /= static_cast<uint64_t>(QpcFrequency);

		const uint32_t lastFrameCount = FrameCount;
		if (IsFixedTimeStep)
		{
			if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(delta - TargetElapsedTicks))) < TicksPerSecond /
				4000)
			{
				delta = TargetElapsedTicks;
			}

			LeftOverTicks += delta;
			while (LeftOverTicks >= TargetElapsedTicks)
			{
				ElapsedTicks = TargetElapsedTicks;
				TotalTicks += TargetElapsedTicks;
				LeftOverTicks -= TargetElapsedTicks;
				FrameCount++;

				update();
			}
		}
		else
		{
			ElapsedTicks = delta;
			TotalTicks += delta;
			LeftOverTicks = 0;
			FrameCount++;

			update();
		}

		if (FrameCount != lastFrameCount)
		{
			FramesThisSecond++;
		}

		if (QpcSecondCounter >= QpcFrequency)
		{
			FramesPerSecond = FramesThisSecond;
			FramesThisSecond = 0;
			QpcSecondCounter %= QpcFrequency;
		}
	}

	static constexpr double TicksToSeconds(const uint64_t ticks) noexcept
	{
		return static_cast<double>(ticks) / TicksPerSecond;
	}

	static constexpr uint64_t SecondsToTicks(const double seconds) noexcept
	{
		return static_cast<uint64_t>(seconds * TicksPerSecond);
	}

private:
	uint64_t QpcFrequency;
	uint64_t QpcLastTime;
	uint64_t QpcMaxDelta;
	uint64_t QpcSecondCounter;
	uint64_t ElapsedTicks;
	uint64_t TotalTicks;
	uint64_t LeftOverTicks;
	uint32_t FrameCount;
	uint32_t FramesPerSecond;
	uint32_t FramesThisSecond;
	bool IsFixedTimeStep;
	uint64_t TargetElapsedTicks;
};
