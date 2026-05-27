#pragma once

#include "DesignTypes.h"

namespace ucav {

class WeightAnalysis {
public:
    WeightResult solve(const DesignRequirements& requirements,
                       const AeroParameters& aero,
                       const EngineParameters& engine,
                       const std::vector<MissionSegment>& mission) const;

    double emptyWeightFraction(double takeoffWeightKg) const;
    double segmentFuelFraction(const MissionSegment& segment,
                               const AeroParameters& aero,
                               const EngineParameters& engine) const;
};

} // namespace ucav
