#pragma once

#include <algorithm>
#include <unordered_map>

namespace LevelProgress
{
    inline std::unordered_map<int, int> bestStars;
    inline std::unordered_map<int, float> bestTimes;

    inline void recordStars(int levelIndex, int stars)
    {
        bestStars[levelIndex] = std::max(bestStars[levelIndex], stars);
    }

    inline int getStars(int levelIndex)
    {
        const auto result = bestStars.find(levelIndex);
        return result == bestStars.end() ? 0 : result->second;
    }

    inline void recordTime(int levelIndex, float time)
    {
        const auto result = bestTimes.find(levelIndex);
        if (result == bestTimes.end() || time < result->second)
            bestTimes[levelIndex] = time;
    }

    inline float getBestTime(int levelIndex)
    {
        const auto result = bestTimes.find(levelIndex);
        return result == bestTimes.end() ? 0.f : result->second;
    }
}
