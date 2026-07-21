#include "NumericalBallisticsEngine.h"
#include "IEnvironmentModel.h"
#include "Ballistics.h"

FVector FNumericalBallisticsEngine::ComputeAcceleration(
	const FVector& PositionEnu,
	const FVector& VelocityEnu,
	const FBallisticProjectileParams& Projectile,
	const IEnvironmentModel& Environment,
	double GravityMps2)
{
	const FVector Gravity(0.0, 0.0, -GravityMps2);

	// Drag acts opposite the velocity of the projectile relative to the air.
	const double Altitude = PositionEnu.Z;
	const FVector Wind = Environment.GetWindVelocityEnu(Altitude);
	const FVector RelativeVelocity = VelocityEnu - Wind;
	const double RelativeSpeed = RelativeVelocity.Size();

	if (RelativeSpeed <= UE_DOUBLE_SMALL_NUMBER)
	{
		return Gravity;
	}

	const double AirDensity = Environment.GetAirDensityKgM3(Altitude);

	// a_drag = -0.5 * rho * |v_rel| * v_rel / BC   (BC = mass / (Cd * A), kg/m^2)
	const FVector DragAcceleration =
		-0.5 * AirDensity * RelativeSpeed * RelativeVelocity / Projectile.BallisticCoefficient;

	// Coriolis / earth-rotation would be added here in a future extension.
	return Gravity + DragAcceleration;
}

TSisaResult<FBallisticTrajectory> FNumericalBallisticsEngine::Solve(
	const FBallisticProjectileParams& Projectile,
	const FBallisticLaunchParams& Launch,
	const IEnvironmentModel& Environment,
	const FBallisticsSolveConfig& Config) const
{
	// --- Input validation -------------------------------------------------
	if (Projectile.InitialSpeedMps <= 0.0)
	{
		return TSisaResult<FBallisticTrajectory>::Err(TEXT("Ballistics.InvalidInput"), TEXT("Initial speed must be greater than zero."));
	}
	if (Projectile.MassKg <= 0.0)
	{
		return TSisaResult<FBallisticTrajectory>::Err(TEXT("Ballistics.InvalidInput"), TEXT("Mass must be greater than zero."));
	}
	if (Projectile.BallisticCoefficient <= 0.0)
	{
		return TSisaResult<FBallisticTrajectory>::Err(TEXT("Ballistics.InvalidInput"), TEXT("Ballistic coefficient must be greater than zero."));
	}
	if (Launch.ElevationDegrees < 0.0 || Launch.ElevationDegrees > 90.0)
	{
		return TSisaResult<FBallisticTrajectory>::Err(TEXT("Ballistics.InvalidInput"), TEXT("Elevation must be within [0, 90] degrees."));
	}
	if (Config.IntegrationTimeStepSeconds <= 0.0 || Config.MaxFlightTimeSeconds <= 0.0)
	{
		return TSisaResult<FBallisticTrajectory>::Err(TEXT("Ballistics.InvalidInput"), TEXT("Integration step and max flight time must be positive."));
	}

	// --- Initial state ----------------------------------------------------
	const double AzimuthRad = FMath::DegreesToRadians(Launch.AzimuthDegrees);
	const double ElevationRad = FMath::DegreesToRadians(Launch.ElevationDegrees);
	const double HorizontalSpeed = Projectile.InitialSpeedMps * FMath::Cos(ElevationRad);

	// ENU: X=East, Y=North, Z=Up. Azimuth is clockwise from North.
	FVector Position = FVector::ZeroVector;
	FVector Velocity(
		HorizontalSpeed * FMath::Sin(AzimuthRad),
		HorizontalSpeed * FMath::Cos(AzimuthRad),
		Projectile.InitialSpeedMps * FMath::Sin(ElevationRad));

	const double Dt = Config.IntegrationTimeStepSeconds;
	const double ImpactPlane = Config.ImpactPlaneHeightMeters;

	FBallisticTrajectory Trajectory;
	Trajectory.Points.Add(FTrajectoryPoint(0.0, Position, Velocity));

	double Time = 0.0;
	double LastSampleTime = 0.0;
	double MaxHeight = Position.Z;
	bool bImpacted = false;

	// --- Integration loop -------------------------------------------------
	while (Time < Config.MaxFlightTimeSeconds)
	{
		const FVector PrevPosition = Position;
		const FVector PrevVelocity = Velocity;
		const double PrevTime = Time;

		// Classic RK4 over the coupled (position, velocity) system.
		const FVector K1V = ComputeAcceleration(Position, Velocity, Projectile, Environment, Config.GravityMps2);
		const FVector K1P = Velocity;

		const FVector K2V = ComputeAcceleration(Position + 0.5 * Dt * K1P, Velocity + 0.5 * Dt * K1V, Projectile, Environment, Config.GravityMps2);
		const FVector K2P = Velocity + 0.5 * Dt * K1V;

		const FVector K3V = ComputeAcceleration(Position + 0.5 * Dt * K2P, Velocity + 0.5 * Dt * K2V, Projectile, Environment, Config.GravityMps2);
		const FVector K3P = Velocity + 0.5 * Dt * K2V;

		const FVector K4V = ComputeAcceleration(Position + Dt * K3P, Velocity + Dt * K3V, Projectile, Environment, Config.GravityMps2);
		const FVector K4P = Velocity + Dt * K3V;

		Position += (Dt / 6.0) * (K1P + 2.0 * K2P + 2.0 * K3P + K4P);
		Velocity += (Dt / 6.0) * (K1V + 2.0 * K2V + 2.0 * K3V + K4V);
		Time += Dt;

		MaxHeight = FMath::Max(MaxHeight, Position.Z);

		// Impact: crossing the impact plane on the way down (or already below it).
		if (PrevPosition.Z >= ImpactPlane && Position.Z <= ImpactPlane)
		{
			double Alpha = 0.0;
			const double Denominator = PrevPosition.Z - Position.Z;
			if (FMath::Abs(Denominator) > UE_DOUBLE_SMALL_NUMBER)
			{
				Alpha = FMath::Clamp((PrevPosition.Z - ImpactPlane) / Denominator, 0.0, 1.0);
			}

			const FVector ImpactPosition = FMath::Lerp(PrevPosition, Position, Alpha);
			const FVector ImpactVelocity = FMath::Lerp(PrevVelocity, Velocity, Alpha);
			const double ImpactTime = FMath::Lerp(PrevTime, Time, Alpha);

			Trajectory.Points.Add(FTrajectoryPoint(ImpactTime, ImpactPosition, ImpactVelocity));
			Trajectory.TimeOfFlightSeconds = ImpactTime;
			Trajectory.ImpactPointEnu = ImpactPosition;
			Trajectory.ImpactVelocityEnu = ImpactVelocity;
			Trajectory.ResidualSpeedMps = ImpactVelocity.Size();
			Trajectory.RangeMeters = FVector(ImpactPosition.X, ImpactPosition.Y, 0.0).Size();
			Trajectory.ImpactEnergyJoules = 0.5 * Projectile.MassKg * ImpactVelocity.SizeSquared();
			bImpacted = true;
			break;
		}

		// Record a sampled point roughly every OutputSampleIntervalSeconds.
		if (Time - LastSampleTime >= Config.OutputSampleIntervalSeconds)
		{
			Trajectory.Points.Add(FTrajectoryPoint(Time, Position, Velocity));
			LastSampleTime = Time;
		}
	}

	Trajectory.MaxHeightMeters = MaxHeight;
	Trajectory.bImpacted = bImpacted;

	if (!bImpacted)
	{
		// Never crossed the impact plane within the time cap: report the last state.
		Trajectory.TimeOfFlightSeconds = Time;
		Trajectory.ImpactPointEnu = Position;
		Trajectory.ImpactVelocityEnu = Velocity;
		Trajectory.ResidualSpeedMps = Velocity.Size();
		Trajectory.RangeMeters = FVector(Position.X, Position.Y, 0.0).Size();
		Trajectory.ImpactEnergyJoules = 0.5 * Projectile.MassKg * Velocity.SizeSquared();
		UE_LOG(LogSisaBallistics, Warning,
			TEXT("Ballistic solve reached the %.0fs time cap without impact (elevation %.1f deg, speed %.0f m/s)."),
			Config.MaxFlightTimeSeconds, Launch.ElevationDegrees, Projectile.InitialSpeedMps);
	}

	return TSisaResult<FBallisticTrajectory>::Ok(Trajectory);
}
