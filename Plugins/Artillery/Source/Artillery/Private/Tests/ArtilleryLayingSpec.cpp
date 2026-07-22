#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ArtilleryLaying.h"

DEFINE_SPEC(
	FArtilleryLayingSpec,
	"SISA.Artillery.Laying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	FArtilleryPieceDefinition MakeMount(double TraverseHalfArc, double TraverseRate, double ElevationRate)
	{
		FArtilleryPieceDefinition Piece;
		Piece.PieceId = TEXT("TestMount");
		Piece.TraverseHalfArcDegrees = TraverseHalfArc;
		Piece.MinElevationDegrees = 0.0;
		Piece.MaxElevationDegrees = 60.0;
		Piece.TraverseRateDegreesPerSecond = TraverseRate;
		Piece.ElevationRateDegreesPerSecond = ElevationRate;
		return Piece;
	}
}

void FArtilleryLayingSpec::Define()
{
	Describe("Angle helpers", [this]()
	{
		It("normalizes azimuths into [0, 360)", [this]()
		{
			TestEqual(TEXT("370 -> 10"), FArtilleryLayingSolver::NormalizeAzimuth(370.0), 10.0, 1e-9);
			TestEqual(TEXT("-10 -> 350"), FArtilleryLayingSolver::NormalizeAzimuth(-10.0), 350.0, 1e-9);
			TestEqual(TEXT("360 -> 0"), FArtilleryLayingSolver::NormalizeAzimuth(360.0), 0.0, 1e-9);
		});

		It("takes the shortest way around the compass", [this]()
		{
			TestEqual(TEXT("350 -> 10 is +20"), FArtilleryLayingSolver::ShortestAzimuthDelta(350.0, 10.0), 20.0, 1e-9);
			TestEqual(TEXT("10 -> 350 is -20"), FArtilleryLayingSolver::ShortestAzimuthDelta(10.0, 350.0), -20.0, 1e-9);
			TestEqual(TEXT("0 -> 90 is +90"), FArtilleryLayingSolver::ShortestAzimuthDelta(0.0, 90.0), 90.0, 1e-9);
		});
	});

	Describe("Traverse arc", [this]()
	{
		It("clamps a request beyond the arc to its edge", [this]()
		{
			// Mount laid on 90 degrees, 25 degrees of traverse either side.
			TestEqual(TEXT("Clamped to +25"), FArtilleryLayingSolver::ClampAzimuthToArc(180.0, 90.0, 25.0), 115.0, 1e-9);
			TestEqual(TEXT("Clamped to -25"), FArtilleryLayingSolver::ClampAzimuthToArc(0.0, 90.0, 25.0), 65.0, 1e-9);
		});

		It("leaves a request inside the arc untouched", [this]()
		{
			TestEqual(TEXT("Within arc"), FArtilleryLayingSolver::ClampAzimuthToArc(100.0, 90.0, 25.0), 100.0, 1e-9);
		});

		It("handles an arc that wraps past North", [this]()
		{
			// Mount on 10 degrees: the arc spans 345..35, crossing 0.
			TestEqual(TEXT("350 stays"), FArtilleryLayingSolver::ClampAzimuthToArc(350.0, 10.0, 25.0), 350.0, 1e-9);
			TestEqual(TEXT("300 clamps to 345"), FArtilleryLayingSolver::ClampAzimuthToArc(300.0, 10.0, 25.0), 345.0, 1e-9);
			TestTrue(TEXT("350 is within arc"), FArtilleryLayingSolver::IsAzimuthWithinArc(350.0, 10.0, 25.0));
			TestFalse(TEXT("300 is outside arc"), FArtilleryLayingSolver::IsAzimuthWithinArc(300.0, 10.0, 25.0));
		});

		It("accepts any azimuth on a fully-traversing mount", [this]()
		{
			TestEqual(TEXT("Untouched"), FArtilleryLayingSolver::ClampAzimuthToArc(270.0, 0.0, 180.0), 270.0, 1e-9);
			TestTrue(TEXT("Always within arc"), FArtilleryLayingSolver::IsAzimuthWithinArc(270.0, 0.0, 180.0));
		});
	});

	Describe("OrderLaying", [this]()
	{
		It("clamps the order to the mechanical limits without moving the tube", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeMount(25.0, 5.0, 3.0);
			FArtilleryLayingState State;
			State.MountAzimuthDegrees = 90.0;
			State.CurrentAzimuthDegrees = 90.0;

			const FArtilleryLayingState Ordered = FArtilleryLayingSolver::OrderLaying(State, Piece, 200.0, 80.0);

			TestEqual(TEXT("Azimuth clamped to arc"), Ordered.TargetAzimuthDegrees, 115.0, 1e-9);
			TestEqual(TEXT("Elevation clamped to max"), Ordered.TargetElevationDegrees, 60.0, 1e-9);
			TestEqual(TEXT("Tube has not moved"), Ordered.CurrentAzimuthDegrees, 90.0, 1e-9);
		});
	});

	Describe("Step", [this]()
	{
		It("slews at no more than the declared rates", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeMount(180.0, 5.0, 3.0);
			FArtilleryLayingState State;
			State.TargetAzimuthDegrees = 90.0;
			State.TargetElevationDegrees = 45.0;

			const FArtilleryLayingState Stepped = FArtilleryLayingSolver::Step(State, Piece, 1.0);

			TestEqual(TEXT("Azimuth advanced 5 deg"), Stepped.CurrentAzimuthDegrees, 5.0, 1e-9);
			TestEqual(TEXT("Elevation advanced 3 deg"), Stepped.CurrentElevationDegrees, 3.0, 1e-9);
			TestFalse(TEXT("Not on target yet"), FArtilleryLayingSolver::IsOnTarget(Stepped));
		});

		It("never overshoots the ordered laying", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeMount(180.0, 5.0, 3.0);
			FArtilleryLayingState State;
			State.TargetAzimuthDegrees = 2.0;
			State.TargetElevationDegrees = 1.0;

			const FArtilleryLayingState Stepped = FArtilleryLayingSolver::Step(State, Piece, 10.0);

			TestEqual(TEXT("Azimuth exact"), Stepped.CurrentAzimuthDegrees, 2.0, 1e-9);
			TestEqual(TEXT("Elevation exact"), Stepped.CurrentElevationDegrees, 1.0, 1e-9);
			TestTrue(TEXT("On target"), FArtilleryLayingSolver::IsOnTarget(Stepped));
		});

		It("slews the short way across North", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeMount(180.0, 5.0, 3.0);
			FArtilleryLayingState State;
			State.CurrentAzimuthDegrees = 355.0;
			State.TargetAzimuthDegrees = 5.0;

			const FArtilleryLayingState Stepped = FArtilleryLayingSolver::Step(State, Piece, 1.0);

			TestEqual(TEXT("Crossed 0 forwards"), Stepped.CurrentAzimuthDegrees, 0.0, 1e-9);
		});

		It("reaches the ordered laying in the expected number of steps", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeMount(180.0, 5.0, 3.0);
			FArtilleryLayingState State;
			State.TargetAzimuthDegrees = 50.0;   // 10 s at 5 deg/s
			State.TargetElevationDegrees = 45.0; // 15 s at 3 deg/s

			// 0.1 s steps for 15 s: elevation is the slower axis and sets the pace.
			for (int32 Step = 0; Step < 150; ++Step)
			{
				State = FArtilleryLayingSolver::Step(State, Piece, 0.1);
			}

			TestTrue(TEXT("On target after 15 s"), FArtilleryLayingSolver::IsOnTarget(State));
			TestEqual(TEXT("Azimuth reached"), State.CurrentAzimuthDegrees, 50.0, 1e-6);
			TestEqual(TEXT("Elevation reached"), State.CurrentElevationDegrees, 45.0, 1e-6);
		});

		It("does nothing for a non-positive delta time", [this]()
		{
			const FArtilleryPieceDefinition Piece = MakeMount(180.0, 5.0, 3.0);
			FArtilleryLayingState State;
			State.TargetAzimuthDegrees = 90.0;

			const FArtilleryLayingState Stepped = FArtilleryLayingSolver::Step(State, Piece, 0.0);
			TestEqual(TEXT("Unmoved"), Stepped.CurrentAzimuthDegrees, 0.0, 1e-9);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
