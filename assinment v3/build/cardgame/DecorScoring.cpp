#include "DecorScoring.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

double DecorScoring::computeRawIDF(int totalTeams, int teamsUsingItem) {
    if (totalTeams <= 1) return 0.0;
    const int clampedDf = std::clamp(teamsUsingItem, 1, totalTeams);
    return std::log(static_cast<double>(totalTeams) / static_cast<double>(clampedDf));
}

double DecorScoring::computeNormalizedIDF(int totalTeams, int teamsUsingItem) {
    if (totalTeams <= 1) return 1.0;
    const double maxIdf = std::log(static_cast<double>(totalTeams));
    return computeRawIDF(totalTeams, teamsUsingItem) / maxIdf;
}

void DecorScoring::printOriginalityReport(int totalTeams,
                                          const std::vector<DecorItemRecord>& items,
                                          double maxDecorPoints) {
    int modelCount = 0;
    double normSum = 0.0;

    std::cout << "\n============= Decorative Object IDF Scoring Report =============\n";
    std::cout << "Total Teams (N): " << totalTeams << "\n";
    std::cout << std::left << std::setw(30) << "Decorative Object"
              << std::setw(12) << "Geometry"
              << std::setw(8)  << "df"
              << std::setw(12) << "Raw IDF"
              << std::setw(12) << "Norm Weight" << "\n";
    std::cout << std::string(74, '-') << "\n";

    for (const auto& item : items) {
        if (item.uses3DModel) ++modelCount;
        const double raw = computeRawIDF(totalTeams, item.teamsUsingItem);
        const double norm = computeNormalizedIDF(totalTeams, item.teamsUsingItem);
        normSum += norm;

        std::cout << std::left << std::setw(30) << item.name
                  << std::setw(12) << (item.uses3DModel ? "3D Model" : "Primitive")
                  << std::setw(8)  << item.teamsUsingItem
                  << std::setw(12) << std::fixed << std::setprecision(4) << raw
                  << std::setw(12) << norm << "\n";
    }

    const double avgNorm = items.empty() ? 0.0 : (normSum / static_cast<double>(items.size()));
    const double contribution = avgNorm * maxDecorPoints;

    std::cout << std::string(74, '-') << "\n";
    std::cout << "3D Model Limit (max 1 of 3): " << modelCount << " / 1 -> "
              << (modelCount <= 1 ? "PASS" : "VIOLATION") << "\n";
    std::cout << "Average Originality Weight: " << std::fixed << std::setprecision(3) << avgNorm
              << " | Estimated Decor Score: " << contribution << " / " << maxDecorPoints << "\n";
    std::cout << "================================================================\n\n";
}
