#include "../src/core/ConstraintAnalysis.h"
#include "../src/core/WeightAnalysis.h"

#include <cassert>
#include <iostream>

int main()
{
    ucav::DesignRequirements req;
    ucav::AeroParameters aero;
    ucav::EngineParameters engine;
    const std::vector<ucav::MissionSegment> mission = ucav::defaultMission(req);

    ucav::WeightAnalysis weight;
    const ucav::WeightResult weightResult = weight.solve(req, aero, engine, mission);
    assert(weightResult.takeoffWeightKg > 5000.0);
    assert(weightResult.takeoffWeightKg < 80000.0);
    assert(weightResult.fuelFraction > 0.05);
    assert(weightResult.fuelFraction < 0.55);
    assert(weightResult.segmentResults.size() == mission.size());

    ucav::ConstraintAnalysis constraints;
    const ucav::ConstraintResult constraintResult = constraints.solve(req, aero, engine);
    assert(constraintResult.curves.size() == 5);
    for (const ucav::ConstraintCurve& curve : constraintResult.curves) {
        assert(!curve.points.empty());
    }
    assert(constraintResult.recommendedWingLoading > 1500.0);
    assert(constraintResult.recommendedThrustToWeight > 0.1);

    const ucav::MarginResult margins = constraints.margins(
        constraintResult.recommendedWingLoading,
        constraintResult.recommendedThrustToWeight,
        req,
        aero,
        engine);
    assert(margins.cruiseMarginPercent > -1.0);
    assert(margins.takeoffMarginPercent > -1.0);

    std::cout << "MTOW kg: " << weightResult.takeoffWeightKg << "\n";
    std::cout << "Fuel fraction: " << weightResult.fuelFraction << "\n";
    std::cout << "Recommended W/S: " << constraintResult.recommendedWingLoading << "\n";
    std::cout << "Recommended T/W: " << constraintResult.recommendedThrustToWeight << "\n";
    std::cout << "Core tests passed\n";
    return 0;
}
