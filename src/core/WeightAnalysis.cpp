#include "WeightAnalysis.h"

#include <algorithm>
#include <cmath>

namespace ucav {

namespace {
constexpr double kSoundSpeedCruiseMs = 295.0;
constexpr double kReserveFuelFraction = 0.06;

double clamp(double value, double low, double high)
{
    return std::max(low, std::min(value, high));
}
}

std::vector<MissionSegment> defaultMission(const DesignRequirements& req)
{
    return {
        {MissionSegmentType::WarmupTakeoff, "暖机/起飞", 0.0, 0.0, 0.0, 0.0, 0.0, 0.970},
        {MissionSegmentType::Climb, "爬升", 0.0, 0.0, req.cruiseAltitudeM, 0.55, 0.0, 0.985},
        {MissionSegmentType::Cruise, "出航巡航", req.combatRadiusKm, 0.0, req.cruiseAltitudeM, req.cruiseMach, 0.0, 0.0},
        {MissionSegmentType::Loiter, "盘旋侦察", 0.0, 30.0, req.cruiseAltitudeM, 0.45, 0.0, 0.0},
        {MissionSegmentType::Approach, "低空进近", 120.0, 0.0, 1000.0, 0.35, 0.0, 0.990},
        {MissionSegmentType::PayloadDrop, "投弹", 0.0, 0.0, 0.0, 0.0, req.payloadKg, 1.000},
        {MissionSegmentType::ReturnCruise, "返航巡航", req.combatRadiusKm, 0.0, req.cruiseAltitudeM, req.cruiseMach, 0.0, 0.0},
        {MissionSegmentType::Landing, "着陆", 0.0, 0.0, 0.0, 0.0, 0.0, 0.995}
    };
}

std::string missionSegmentTypeName(MissionSegmentType type)
{
    switch (type) {
    case MissionSegmentType::WarmupTakeoff: return "暖机/起飞";
    case MissionSegmentType::Climb: return "爬升";
    case MissionSegmentType::Cruise: return "出航巡航";
    case MissionSegmentType::Loiter: return "盘旋侦察";
    case MissionSegmentType::Approach: return "低空进近";
    case MissionSegmentType::PayloadDrop: return "投弹";
    case MissionSegmentType::ReturnCruise: return "返航巡航";
    case MissionSegmentType::Landing: return "着陆";
    }
    return "未知";
}

MissionSegmentType missionSegmentTypeFromIndex(int index)
{
    const int bounded = static_cast<int>(clamp(index, 0, 7));
    return static_cast<MissionSegmentType>(bounded);
}

int missionSegmentTypeIndex(MissionSegmentType type)
{
    return static_cast<int>(type);
}

double WeightAnalysis::emptyWeightFraction(double takeoffWeightKg) const
{
    const double weightLb = std::max(1000.0, takeoffWeightKg * 2.20462);
    return clamp(1.02 * std::pow(weightLb, -0.06), 0.38, 0.68);
}

double WeightAnalysis::segmentFuelFraction(const MissionSegment& segment,
                                           const AeroParameters& aero,
                                           const EngineParameters& engine) const
{
    if (segment.fixedFuelFraction > 0.0) {
        return clamp(segment.fixedFuelFraction, 0.80, 1.0);
    }

    const double liftDrag = std::max(5.0, aero.maxLiftDrag);
    if (segment.type == MissionSegmentType::Cruise ||
        segment.type == MissionSegmentType::ReturnCruise ||
        (segment.distanceKm > 0.0 && segment.durationMin <= 0.0)) {
        const double mach = segment.mach > 0.0 ? segment.mach : 0.75;
        const double speedKmh = mach * kSoundSpeedCruiseMs * 3.6;
        const double enduranceHours = segment.distanceKm / std::max(200.0, speedKmh);
        return clamp(std::exp(-engine.cruiseSfcPerHour * enduranceHours / liftDrag), 0.70, 1.0);
    }

    if (segment.type == MissionSegmentType::Loiter || segment.durationMin > 0.0) {
        const double enduranceHours = segment.durationMin / 60.0;
        return clamp(std::exp(-engine.loiterSfcPerHour * enduranceHours / liftDrag), 0.80, 1.0);
    }

    return 0.995;
}

WeightResult WeightAnalysis::solve(const DesignRequirements& requirements,
                                   const AeroParameters& aero,
                                   const EngineParameters& engine,
                                   const std::vector<MissionSegment>& mission) const
{
    WeightResult result;
    double takeoffWeightKg = 18000.0;

    for (int i = 0; i < 80; ++i) {
        double currentWeightKg = takeoffWeightKg;
        double fuelUsedKg = 0.0;

        for (const MissionSegment& segment : mission) {
            const double before = currentWeightKg;
            const double fraction = segmentFuelFraction(segment, aero, engine);
            currentWeightKg *= fraction;
            fuelUsedKg += before - currentWeightKg;
            currentWeightKg = std::max(1.0, currentWeightKg - segment.payloadDropKg);
        }

        const double missionFuelFraction = clamp(fuelUsedKg / takeoffWeightKg + kReserveFuelFraction, 0.05, 0.55);
        const double emptyFraction = emptyWeightFraction(takeoffWeightKg);
        const double denominator = 1.0 - emptyFraction - missionFuelFraction;
        const double usefulFixedWeight = requirements.payloadKg + requirements.crewAndFixedKg;
        const double nextWeightKg = denominator > 0.05 ? usefulFixedWeight / denominator : takeoffWeightKg * 1.1;
        const double residual = nextWeightKg - takeoffWeightKg;

        result.iterations.push_back({i + 1, takeoffWeightKg, emptyFraction, missionFuelFraction, residual});
        takeoffWeightKg = 0.55 * takeoffWeightKg + 0.45 * nextWeightKg;

        if (std::abs(residual) < 1.0) {
            result.converged = true;
            break;
        }
    }

    result.takeoffWeightKg = takeoffWeightKg;
    result.emptyFraction = emptyWeightFraction(takeoffWeightKg);

    double currentWeightKg = takeoffWeightKg;
    double fuelUsedKg = 0.0;
    for (const MissionSegment& segment : mission) {
        const double before = currentWeightKg;
        const double fraction = segmentFuelFraction(segment, aero, engine);
        currentWeightKg *= fraction;
        fuelUsedKg += before - currentWeightKg;
        currentWeightKg = std::max(1.0, currentWeightKg - segment.payloadDropKg);
        result.segmentResults.push_back({segment.name, fraction, before, currentWeightKg});
    }

    result.fuelFraction = clamp(fuelUsedKg / takeoffWeightKg + kReserveFuelFraction, 0.05, 0.55);
    result.emptyWeightKg = result.emptyFraction * takeoffWeightKg;
    result.fuelWeightKg = result.fuelFraction * takeoffWeightKg;
    return result;
}

} // namespace ucav
