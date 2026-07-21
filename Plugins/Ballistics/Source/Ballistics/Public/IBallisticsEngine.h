#pragma once

#include "CoreMinimal.h"
#include "SisaResult.h"
#include "BallisticTypes.h"

class IEnvironmentModel;

/**
 * The contract for a ballistic trajectory solver. A plain C++ interface (not a
 * UINTERFACE) because Solve returns TSisaResult<T>, a template that Unreal
 * Header Tool cannot reflect. Consumers depend on this contract rather than on
 * the concrete numerical implementation (DIP); UBallisticsSubsystem owns the
 * default implementation and exposes it to other plugins.
 */
class IBallisticsEngine
{
public:
	virtual ~IBallisticsEngine() = default;

	/**
	 * Solves the flight of a projectile in a local ENU frame (muzzle at origin).
	 * Returns an error result if the inputs are physically invalid (non-positive
	 * speed/mass/ballistic-coefficient, or elevation outside [0, 90] degrees).
	 */
	virtual TSisaResult<FBallisticTrajectory> Solve(
		const FBallisticProjectileParams& Projectile,
		const FBallisticLaunchParams& Launch,
		const IEnvironmentModel& Environment,
		const FBallisticsSolveConfig& Config) const = 0;
};
