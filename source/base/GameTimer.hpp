#pragma once

#include <SDL.h>

#include <algorithm>


class GameTimer
{
	static constexpr uint64_t TICKS_PER_SECOND = 10000000;

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

		uint64_t delta = currentTime - m_qpcLastTime;
		m_qpcLastTime = currentTime;
		m_qpcSecondCounter += delta;

		delta = std::clamp(delta, static_cast<uint64_t>(0), m_qpcMaxDelta);
		delta *= TICKS_PER_SECOND;
		delta /= static_cast<uint64_t>(m_qpcFrequency);

		const uint32_t lastFrameCount = m_frameCount;
		if (m_isFixedTimeStep)
		{
			if (static_cast<uint64_t>(std::abs(static_cast<int64_t>(delta - m_targetElapsedTicks))) < TICKS_PER_SECOND /
				4000)
			{
				delta = m_targetElapsedTicks;
			}

			m_leftOverTicks += delta;
			while (m_leftOverTicks >= m_targetElapsedTicks)
			{
				m_elapsedTicks = m_targetElapsedTicks;
				m_totalTicks += m_targetElapsedTicks;
				m_leftOverTicks -= m_targetElapsedTicks;
				m_frameCount++;

				update();
			}
		}
		else
		{
			m_elapsedTicks = delta;
			m_totalTicks += delta;
			m_leftOverTicks = 0;
			m_frameCount++;

			update();
		}

		if (m_frameCount != lastFrameCount)
		{
			m_framesThisSecond++;
		}

		if (m_qpcSecondCounter >= m_qpcFrequency)
		{
			m_framesPerSecond = m_framesThisSecond;
			m_framesThisSecond = 0;
			m_qpcSecondCounter %= m_qpcFrequency;
		}
	}

	static constexpr double TicksToSeconds(const uint64_t ticks) noexcept
	{
		return static_cast<double>(ticks) / TICKS_PER_SECOND;
	}

	static constexpr uint64_t SecondsToTicks(const double seconds) noexcept
	{
		return static_cast<uint64_t>(seconds * TICKS_PER_SECOND);
	}

private:
	uint64_t m_qpcFrequency;
	uint64_t m_qpcLastTime;
	uint64_t m_qpcMaxDelta;

	uint64_t m_elapsedTicks;
	uint64_t m_totalTicks;
	uint64_t m_leftOverTicks;

	uint32_t m_frameCount;
	uint32_t m_framesPerSecond;
	uint32_t m_framesThisSecond;
	uint64_t m_qpcSecondCounter;

	bool m_isFixedTimeStep;
	uint64_t m_targetElapsedTicks;
};
