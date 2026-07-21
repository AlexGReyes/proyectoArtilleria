#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NumericalBallisticsEngine.h"
#include "StandardAtmosphereModel.h"
#include "IEnvironmentModel.h"

DEFINE_SPEC(
	FBallisticsEngineSpec,
	"SISA.Ballistics.Engine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	/** Drag-free, wind-free atmosphere so the solver reduces to closed-form projectile motion. */
	class FVacuumEnvironment : public IEnvironmentModel
	{
	public:
		virtual double GetAirDensityKgM3(double) const override { return 0.0; }
		virtual FVector GetWindVelocityEnu(double) const override { return FVector::ZeroVector; }
	};

	FBallisticProjectileParams MakeProjectile()
	{
		FBallisticProjectileParams P;
		P.InitialSpeedMps = 700.0;
		P.MassKg = 43.0;
		P.BallisticCoefficient = 3500.0;
		return P;
	}

	FBallisticLaunchParams MakeLaunch(double AzimuthDeg, double ElevationDeg)
	{
		FBallisticLaunchParams L;
		L.AzimuthDegrees = AzimuthDeg;
		L.ElevationDegrees = ElevationDeg;
		return L;
	}
}

void FBallisticsEngineSpec::Define()
{
	Describe("Vacuum trajectory vs analytic solution", [this]()
	{
		It("matches closed-form range, apogee and time of flight", [this]()
		{
			const FNumericalBallisticsEngine Engine;
			const FVacuumEnvironment Vacuum;
			FBallisticsSolveConfig Config;

			const double V = 700.0;
			const double ElevationDeg = 45.0;
			const double G = Config.GravityMps2;
			const double ElevRad = FMath::DegreesToRadians(ElevationDeg);

			const double ExpectedRange = (V * V * FMath::Sin(2.0 * ElevRad)) / G;
			const double ExpectedApogee = (V * V * FMath::Square(FMath::Sin(ElevRad))) / (2.0 * G);
			const double ExpectedTof = (2.0 * V * FMath::Sin(ElevRad)) / G;

			const TSisaResult<FBallisticTrajectory> Result =
				Engine.Solve(MakeProjectile(), MakeLaunch(0.0, ElevationDeg), Vacuum, Config);
			if (!TestTrue(TEXT("Solve succeeded"), Result.IsOk()))
			{
				return;
			}
			const FBallisticTrajectory& T = Result.GetValue();

			TestTrue(TEXT("Impacted"), T.bImpacted);
			// 0.5% relative tolerance: RK4 is essentially exact for constant acceleration;
			// the small residual is impact-plane interpolation.
			TestTrue(TEXT("Range matches"), FMath::IsNearlyEqual(T.RangeMeters, ExpectedRange, ExpectedRange * 0.005));
			TestTrue(TEXT("Apogee matches"), FMath::IsNearlyEqual(T.MaxHeightMeters, ExpectedApogee, ExpectedApogee * 0.005));
			TestTrue(TEXT("Time of flight matches"), FMath::IsNearlyEqual(T.TimeOfFlightSeconds, ExpectedTof, ExpectedTof * 0.005));
		});
	});

	Describe("Drag", [this]()
	{
		It("reduces range compared to vacuum", [this]()
		{
			const FNumericalBallisticsEngine Engine;
			const FVacuumEnvironment Vacuum;
			const FStandardAtmosphereModel Air(FEnvironmentConditions{});
			FBallisticsSolveConfig Config;

			const TSisaResult<FBallisticTrajectory> VacuumResult = Engine.Solve(MakeProjectile(), MakeLaunch(0.0, 45.0), Vacuum, Config);
			const TSisaResult<FBallisticTrajectory> AirResult = Engine.Solve(MakeProjectile(), MakeLaunch(0.0, 45.0), Air, Config);

			if (!TestTrue(TEXT("Both solves succeeded"), VacuumResult.IsOk() && AirResult.IsOk()))
			{
				return;
			}
			TestTrue(TEXT("Air range is shorter than vacuum range"),
				AirResult.GetValue().RangeMeters < VacuumResult.GetValue().RangeMeters);
		});
	});

	Describe("Wind", [this]()
	{
		auto SolveWithWind = [](double WindFromDeg, double WindSpeed) -> FBallisticTrajectory
		{
			const FNumericalBallisticsEngine Engine;
			FEnvironmentConditions Conditions;
			Conditions.WindSpeedMps = WindSpeed;
			Conditions.WindFromDirectionDegrees = WindFromDeg;
			const FStandardAtmosphereModel Air(Conditions);
			FBallisticsSolveConfig Config;
			// Fire due North (azimuth 0).
			return Engine.Solve(MakeProjectile(), MakeLaunch(0.0, 45.0), Air, Config).GetValue();
		};

		It("extends range with a tailwind and shortens it with a headwind", [this, SolveWithWind]()
		{
			const double NoWindRange = SolveWithWind(0.0, 0.0).RangeMeters;
			const double TailwindRange = SolveWithWind(180.0, 20.0).RangeMeters; // wind from South -> blows North
			const double HeadwindRange = SolveWithWind(0.0, 20.0).RangeMeters;   // wind from North -> blows South

			TestTrue(TEXT("Tailwind increases range"), TailwindRange > NoWindRange);
			TestTrue(TEXT("Headwind decreases range"), HeadwindRange < NoWindRange);
		});

		It("deflects the impact laterally with a crosswind", [this, SolveWithWind]()
		{
			// Wind from the East blows toward the West (-X). Firing North, the round drifts West.
			const FBallisticTrajectory T = SolveWithWind(90.0, 20.0);
			TestTrue(TEXT("Impact drifts to negative East (West)"), T.ImpactPointEnu.X < 0.0);
		});
	});

	Describe("Air density", [this]()
	{
		It("shortens range in denser (colder) air", [this]()
		{
			const FNumericalBallisticsEngine Engine;
			FBallisticsSolveConfig Config;

			FEnvironmentConditions Cold;
			Cold.TemperatureCelsius = -20.0;
			FEnvironmentConditions Hot;
			Hot.TemperatureCelsius = 40.0;

			const FStandardAtmosphereModel ColdAir(Cold);
			const FStandardAtmosphereModel HotAir(Hot);

			const double ColdRange = Engine.Solve(MakeProjectile(), MakeLaunch(0.0, 45.0), ColdAir, Config).GetValue().RangeMeters;
			const double HotRange = Engine.Solve(MakeProjectile(), MakeLaunch(0.0, 45.0), HotAir, Config).GetValue().RangeMeters;

			TestTrue(TEXT("Colder, denser air yields shorter range"), ColdRange < HotRange);
		});
	});

	Describe("Impact energy", [this]()
	{
		It("equals 0.5 * m * v_residual^2", [this]()
		{
			const FNumericalBallisticsEngine Engine;
			const FStandardAtmosphereModel Air(FEnvironmentConditions{});
			FBallisticsSolveConfig Config;
			const FBallisticProjectileParams Projectile = MakeProjectile();

			const FBallisticTrajectory T = Engine.Solve(Projectile, MakeLaunch(0.0, 45.0), Air, Config).GetValue();
			const double Expected = 0.5 * Projectile.MassKg * FMath::Square(T.ResidualSpeedMps);
			TestTrue(TEXT("Energy matches"), FMath::IsNearlyEqual(T.ImpactEnergyJoules, Expected, Expected * 0.001 + 1.0));
		});
	});

	Describe("Input validation", [this]()
	{
		const FStandardAtmosphereModel Air(FEnvironmentConditions{});

		It("rejects non-positive speed", [this, Air]()
		{
			const FNumericalBallisticsEngine Engine;
			FBallisticProjectileParams P = MakeProjectile();
			P.InitialSpeedMps = 0.0;
			TestTrue(TEXT("Error"), Engine.Solve(P, MakeLaunch(0.0, 45.0), Air, FBallisticsSolveConfig{}).IsError());
		});

		It("rejects non-positive mass", [this, Air]()
		{
			const FNumericalBallisticsEngine Engine;
			FBallisticProjectileParams P = MakeProjectile();
			P.MassKg = 0.0;
			TestTrue(TEXT("Error"), Engine.Solve(P, MakeLaunch(0.0, 45.0), Air, FBallisticsSolveConfig{}).IsError());
		});

		It("rejects non-positive ballistic coefficient", [this, Air]()
		{
			const FNumericalBallisticsEngine Engine;
			FBallisticProjectileParams P = MakeProjectile();
			P.BallisticCoefficient = 0.0;
			TestTrue(TEXT("Error"), Engine.Solve(P, MakeLaunch(0.0, 45.0), Air, FBallisticsSolveConfig{}).IsError());
		});

		It("rejects elevation outside [0, 90]", [this, Air]()
		{
			const FNumericalBallisticsEngine Engine;
			TestTrue(TEXT("Error"), Engine.Solve(MakeProjectile(), MakeLaunch(0.0, 120.0), Air, FBallisticsSolveConfig{}).IsError());
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
