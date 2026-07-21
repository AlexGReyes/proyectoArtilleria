#pragma once

#include "CoreMinimal.h"
#include "IBallisticsEngine.h"

/**
 * Fixed-step Runge-Kutta (RK4) implementation of IBallisticsEngine. Integrates
 * the projectile's equations of motion in a local ENU frame under gravity plus
 * aerodynamic drag, querying air density and wind from the supplied
 * IEnvironmentModel at each sub-step. Stateless and free of any UObject/UWorld
 * dependency, so it is unit-testable headless.
 *
 * The acceleration is isolated in ComputeAcceleration(): this is the single
 * place a future Coriolis / earth-rotation term would be added, without
 * touching the integrator or impact logic.
 */
class FNumericalBallisticsEngine : public IBallisticsEngine
{
public:
	virtual TSisaResult<FBallisticTrajectory> Solve(
		const FBallisticProjectileParams& Projectile,
		const FBallisticLaunchParams& Launch,
		const IEnvironmentModel& Environment,
		const FBallisticsSolveConfig& Config) const override;

private:
	/** Total acceleration (m/s^2, ENU) at a given position/velocity under gravity + drag. */
	static FVector ComputeAcceleration(
		const FVector& PositionEnu,
		const FVector& VelocityEnu,
		const FBallisticProjectileParams& Projectile,
		const IEnvironmentModel& Environment,
		double GravityMps2);
};
