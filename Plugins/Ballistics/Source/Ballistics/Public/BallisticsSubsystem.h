#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SisaResult.h"
#include "BallisticTypes.h"
#include "BallisticsSubsystem.generated.h"

class IEnvironmentModel;

/** Callback invoked on the game thread when an async solve finishes. */
using FBallisticsSolveCompleteDelegate = TFunction<void(const TSisaResult<FBallisticTrajectory>&)>;

/**
 * Application-layer entry point to the ballistic solver, and the seam other
 * SISA plugins (Artillery, Simulation) depend on. Offers a synchronous solve
 * for immediate use/tests, and an asynchronous solve that runs the
 * (potentially expensive) integration on a background thread and delivers the
 * result back on the game thread — meeting the project requirement that
 * ballistic computation not block the game thread.
 *
 * The concrete engine (FNumericalBallisticsEngine) is stateless, so it is
 * constructed on demand inside the .cpp rather than stored; consumers depend
 * only on this subsystem's API, not on the numerical implementation.
 */
UCLASS()
class BALLISTICS_API UBallisticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Solves synchronously on the calling thread. Use for tests or cheap one-off solves. */
	TSisaResult<FBallisticTrajectory> SolveSync(
		const FBallisticProjectileParams& Projectile,
		const FBallisticLaunchParams& Launch,
		const IEnvironmentModel& Environment,
		const FBallisticsSolveConfig& Config) const;

	/**
	 * Solves on a background thread and invokes OnComplete on the game thread.
	 * The environment model is passed as a shared reference so it safely outlives
	 * the background work; all other inputs are copied.
	 */
	void SolveAsync(
		const FBallisticProjectileParams& Projectile,
		const FBallisticLaunchParams& Launch,
		const TSharedRef<const IEnvironmentModel>& Environment,
		const FBallisticsSolveConfig& Config,
		FBallisticsSolveCompleteDelegate OnComplete) const;
};
