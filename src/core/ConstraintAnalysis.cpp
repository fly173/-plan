#include "ConstraintAnalysis.h"

#include <algorithm>
#include <cmath>

namespace ucav {

namespace {
constexpr double kRhoSeaLevel = 1.225;
constexpr double kRhoCruise = 0.31;
constexpr double kSoundSpeedCruiseMs = 295.0;

double dynamicPressure(double mach)
{
    const double v = std::max(0.2, mach) * kSoundSpeedCruiseMs;
    return 0.5 * kRhoCruise * v * v;
}

double marginPercent(double selected, double required)
{
    if (required <= 0.0) {
        return 0.0;
    }
    return (selected / required - 1.0) * 100.0;
}
} // namespace

double ConstraintAnalysis::cruiseTW(double wingLoading, const DesignRequirements& requirements,
                                    const AeroParameters& aero, const EngineParameters& engine) const
{
    const double q = dynamicPressure(requirements.cruiseMach);
    const double dragTerm = q * aero.zeroLiftDrag / wingLoading + aero.inducedDragFactor * wingLoading / q;
    return dragTerm / std::max(0.15, engine.thrustLapseCruise);
}

double ConstraintAnalysis::turnTW(double wingLoading, const DesignRequirements& requirements,
                                  const AeroParameters& aero, const EngineParameters& engine) const
{
    const double q = dynamicPressure(0.6);
    const double n = std::max(1.0, requirements.turnLoadFactor);
    const double dragTerm = q * aero.zeroLiftDrag / wingLoading + aero.inducedDragFactor * n * n * wingLoading / q;
    return dragTerm / std::max(0.25, engine.thrustLapseCruise + 0.18);
}

double ConstraintAnalysis::climbTW(double wingLoading, const DesignRequirements& requirements,
                                   const AeroParameters& aero, const EngineParameters& engine) const
{
    const double mach = 0.45;
    const double v = mach * kSoundSpeedCruiseMs;
    const double q = dynamicPressure(mach);
    const double dragTerm = q * aero.zeroLiftDrag / wingLoading + aero.inducedDragFactor * wingLoading / q;
    return (dragTerm + requirements.climbRateMs / std::max(80.0, v)) /
           std::max(0.35, engine.thrustLapseCruise + 0.25);
}

double ConstraintAnalysis::takeoffTW(double wingLoading, const DesignRequirements& requirements,
                                     const AeroParameters& aero) const
{
    const double sigma = 1.0;
    const double denominator = std::max(1.0, 4.2 * sigma * aero.takeoffClMax * requirements.takeoffDistanceM);
    return wingLoading / denominator;
}

double ConstraintAnalysis::landingWingLoadingLimit(const DesignRequirements& requirements,
                                                   const AeroParameters& aero) const
{
    const double approachFactor = 2.2;
    return approachFactor * kRhoSeaLevel * aero.landingClMax * requirements.landingDistanceM;
}

ConstraintResult ConstraintAnalysis::solve(const DesignRequirements& requirements,
                                           const AeroParameters& aero,
                                           const EngineParameters& engine) const
{
    ConstraintResult result;
    ConstraintCurve cruise{"巡航", {}};
    ConstraintCurve turn{"持续盘旋", {}};
    ConstraintCurve climb{"爬升", {}};
    ConstraintCurve takeoff{"起飞距离", {}};
    ConstraintCurve landing{"着陆限制", {}};

    result.landingWingLoadingLimit = landingWingLoadingLimit(requirements, aero);

    for (int i = 0; i <= 80; ++i) {
        const double ws = 1500.0 + i * 100.0;
        cruise.points.push_back({ws, cruiseTW(ws, requirements, aero, engine)});
        turn.points.push_back({ws, turnTW(ws, requirements, aero, engine)});
        climb.points.push_back({ws, climbTW(ws, requirements, aero, engine)});
        takeoff.points.push_back({ws, takeoffTW(ws, requirements, aero)});
        landing.points.push_back({ws, ws <= result.landingWingLoadingLimit ? 0.05 : 1.10});
    }

    result.curves = {cruise, turn, climb, takeoff, landing};

    const double candidateWs = std::min(result.landingWingLoadingLimit * 0.88, 6200.0);
    result.recommendedWingLoading = std::max(2500.0, candidateWs);
    result.recommendedThrustToWeight = std::max({
        cruiseTW(result.recommendedWingLoading, requirements, aero, engine),
        turnTW(result.recommendedWingLoading, requirements, aero, engine),
        climbTW(result.recommendedWingLoading, requirements, aero, engine),
        takeoffTW(result.recommendedWingLoading, requirements, aero)
    }) * 1.12;

    return result;
}

MarginResult ConstraintAnalysis::margins(double selectedWingLoading,
                                         double selectedThrustToWeight,
                                         const DesignRequirements& requirements,
                                         const AeroParameters& aero,
                                         const EngineParameters& engine) const
{
    MarginResult result;
    result.selectedWingLoading = selectedWingLoading;
    result.selectedThrustToWeight = selectedThrustToWeight;
    result.cruiseMarginPercent = marginPercent(selectedThrustToWeight, cruiseTW(selectedWingLoading, requirements, aero, engine));
    result.turnMarginPercent = marginPercent(selectedThrustToWeight, turnTW(selectedWingLoading, requirements, aero, engine));
    result.climbMarginPercent = marginPercent(selectedThrustToWeight, climbTW(selectedWingLoading, requirements, aero, engine));
    result.takeoffMarginPercent = marginPercent(selectedThrustToWeight, takeoffTW(selectedWingLoading, requirements, aero));
    result.landingMarginPercent = (landingWingLoadingLimit(requirements, aero) / selectedWingLoading - 1.0) * 100.0;
    return result;
}

} // namespace ucav
