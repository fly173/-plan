#pragma once

#include <string>
#include <vector>

namespace ucav {

enum class MissionSegmentType {
    WarmupTakeoff,
    Climb,
    Cruise,
    Loiter,
    Approach,
    PayloadDrop,
    ReturnCruise,
    Landing
};

struct DesignRequirements {
    double cruiseMach = 0.8;
    double cruiseAltitudeM = 12000.0;
    double combatRadiusKm = 1600.0;
    double payloadKg = 2000.0;
    double crewAndFixedKg = 450.0;
    double takeoffDistanceM = 1200.0;
    double landingDistanceM = 1100.0;
    double turnLoadFactor = 2.5;
    double climbRateMs = 25.0;
};

struct AeroParameters {
    double maxLiftDrag = 17.0;
    double zeroLiftDrag = 0.022;
    double inducedDragFactor = 0.045;
    double cleanClMax = 1.45;
    double takeoffClMax = 1.8;
    double landingClMax = 2.0;
};

struct EngineParameters {
    double seaLevelStaticThrustN = 65000.0;
    double cruiseSfcPerHour = 0.78;
    double loiterSfcPerHour = 0.66;
    double thrustLapseCruise = 0.42;
};

struct MissionSegment {
    MissionSegmentType type = MissionSegmentType::Cruise;
    std::string name;
    double distanceKm = 0.0;
    double durationMin = 0.0;
    double altitudeM = 0.0;
    double mach = 0.0;
    double payloadDropKg = 0.0;
    double fixedFuelFraction = 0.0;
};

struct WeightIterationRow {
    int iteration = 0;
    double takeoffWeightKg = 0.0;
    double emptyFraction = 0.0;
    double missionFuelFraction = 0.0;
    double residualKg = 0.0;
};

struct MissionSegmentResult {
    std::string name;
    double fuelFraction = 1.0;
    double weightBeforeKg = 0.0;
    double weightAfterKg = 0.0;
};

struct WeightResult {
    bool converged = false;
    double takeoffWeightKg = 0.0;
    double emptyWeightKg = 0.0;
    double fuelWeightKg = 0.0;
    double emptyFraction = 0.0;
    double fuelFraction = 0.0;
    std::vector<WeightIterationRow> iterations;
    std::vector<MissionSegmentResult> segmentResults;
};

struct ConstraintPoint {
    double wingLoading = 0.0;
    double thrustToWeight = 0.0;
};

struct ConstraintCurve {
    std::string name;
    std::vector<ConstraintPoint> points;
};

struct ConstraintResult {
    std::vector<ConstraintCurve> curves;
    double landingWingLoadingLimit = 0.0;
    double recommendedWingLoading = 0.0;
    double recommendedThrustToWeight = 0.0;
};

struct MarginResult {
    double selectedWingLoading = 0.0;
    double selectedThrustToWeight = 0.0;
    double cruiseMarginPercent = 0.0;
    double turnMarginPercent = 0.0;
    double climbMarginPercent = 0.0;
    double takeoffMarginPercent = 0.0;
    double landingMarginPercent = 0.0;
};

std::vector<MissionSegment> defaultMission(const DesignRequirements& req);
std::string missionSegmentTypeName(MissionSegmentType type);
MissionSegmentType missionSegmentTypeFromIndex(int index);
int missionSegmentTypeIndex(MissionSegmentType type);

} // namespace ucav
