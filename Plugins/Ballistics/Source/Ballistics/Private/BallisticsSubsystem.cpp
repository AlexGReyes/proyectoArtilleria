#include "BallisticsSubsystem.h"
#include "NumericalBallisticsEngine.h"
#include "IEnvironmentModel.h"
#include "Async/Async.h"

TSisaResult<FBallisticTrajectory> UBallisticsSubsystem::SolveSync(
	const FBallisticProjectileParams& Projectile,
	const FBallisticLaunchParams& Launch,
	const IEnvironmentModel& Environment,
	const FBallisticsSolveConfig& Config) const
{
	const FNumericalBallisticsEngine Engine;
	return Engine.Solve(Projectile, Launch, Environment, Config);
}

void UBallisticsSubsystem::SolveAsync(
	const FBallisticProjectileParams& Projectile,
	const FBallisticLaunchParams& Launch,
	const TSharedRef<const IEnvironmentModel>& Environment,
	const FBallisticsSolveConfig& Config,
	FBallisticsSolveCompleteDelegate OnComplete) const
{
	// Copy all value inputs into the task; hold the environment alive via the shared ref.
	// The engine is stateless, so the background task constructs its own instance and
	// does not depend on this subsystem's lifetime.
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
		[Projectile, Launch, Environment, Config, OnComplete = MoveTemp(OnComplete)]()
		{
			const FNumericalBallisticsEngine Engine;
			TSisaResult<FBallisticTrajectory> Result = Engine.Solve(Projectile, Launch, Environment.Get(), Config);

			// Marshal the callback back to the game thread.
			AsyncTask(ENamedThreads::GameThread,
				[Result = MoveTemp(Result), OnComplete]()
				{
					if (OnComplete)
					{
						OnComplete(Result);
					}
				});
		});
}
