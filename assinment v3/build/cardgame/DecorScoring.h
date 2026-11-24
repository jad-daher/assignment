#pragma once

#include <string>
#include <vector>

struct DecorItemRecord {
    std::string name;
    bool uses3DModel = false;
    int teamsUsingItem = 1; // Document frequency (df) across all teams
};

class DecorScoring {
public:
    // Raw Inverse Document Frequency: ln(N / df)
    static double computeRawIDF(int totalTeams, int teamsUsingItem);

    // Normalized originality weight in [0.0, 1.0]: ln(N / df) / ln(N)
    static double computeNormalizedIDF(int totalTeams, int teamsUsingItem);

    // Checks the max-1 3D model rule and logs the full IDF contribution breakdown
    static void printOriginalityReport(int totalTeams,
                                       const std::vector<DecorItemRecord>& items,
                                       double maxDecorPoints = 2.0);
};
