#pragma once

#include "DesignTypes.h"

namespace ucav {

class ConstraintAnalysis {
public:
    ConstraintResult solve(const DesignRequirements& requirements,
                           const AeroParameters& aero,
                           const EngineParameters& engine) const;

    MarginResult margins(double selectedWingLoading,
                         double selectedThrustToWeight,
                         const DesignRequirements& requirements,
                         const AeroParameters& aero,
                         const EngineParameters& engine) const;

private:
    double cruiseTW(double wingLoading, const DesignRequirements& requirements,
                    const AeroParameters& aero, const EngineParameters& engine) const;
    double turnTW(double wingLoading, const DesignRequirements& requirements,
                  const AeroParameters& aero, const EngineParameters& engine) const;
    double climbTW(double wingLoading, const DesignRequirements& requirements,
                   const AeroParameters& aero, const EngineParameters& engine) const;
    double takeoffTW(double wingLoading, const DesignRequirements& requirements,
                     const AeroParameters& aero) const;
    double landingWingLoadingLimit(const DesignRequirements& requirements,
                                   const AeroParameters& aero) const;
};

} // namespace ucav
