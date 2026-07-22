#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SisaAngle.h"

DEFINE_SPEC(
	FSisaAngleSpec,
	"SISA.Core.Angle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FSisaAngleSpec::Define()
{
	Describe("Normalize360", [this]()
	{
		It("wraps any angle into [0, 360)", [this]()
		{
			TestEqual(TEXT("370"), SisaAngle::Normalize360(370.0), 10.0, 1e-9);
			TestEqual(TEXT("-10"), SisaAngle::Normalize360(-10.0), 350.0, 1e-9);
			TestEqual(TEXT("360"), SisaAngle::Normalize360(360.0), 0.0, 1e-9);
			TestEqual(TEXT("-720"), SisaAngle::Normalize360(-720.0), 0.0, 1e-9);
			TestEqual(TEXT("Already normalized"), SisaAngle::Normalize360(123.456), 123.456, 1e-9);
		});
	});

	Describe("ShortestDelta", [this]()
	{
		It("takes the shortest way around the compass", [this]()
		{
			TestEqual(TEXT("350 -> 10"), SisaAngle::ShortestDelta(350.0, 10.0), 20.0, 1e-9);
			TestEqual(TEXT("10 -> 350"), SisaAngle::ShortestDelta(10.0, 350.0), -20.0, 1e-9);
			TestEqual(TEXT("0 -> 180"), SisaAngle::ShortestDelta(0.0, 180.0), 180.0, 1e-9);
			TestEqual(TEXT("Identity"), SisaAngle::ShortestDelta(42.0, 42.0), 0.0, 1e-9);
		});
	});

	Describe("StepTowards", [this]()
	{
		It("advances by at most the given step", [this]()
		{
			TestEqual(TEXT("Limited"), SisaAngle::StepTowards(0.0, 90.0, 5.0), 5.0, 1e-9);
			TestEqual(TEXT("Limited backwards"), SisaAngle::StepTowards(0.0, 270.0, 5.0), 355.0, 1e-9);
		});

		It("never overshoots and normalizes the result", [this]()
		{
			TestEqual(TEXT("Reaches target"), SisaAngle::StepTowards(0.0, 3.0, 100.0), 3.0, 1e-9);
			TestEqual(TEXT("Crosses North"), SisaAngle::StepTowards(355.0, 5.0, 10.0), 5.0, 1e-9);
			TestEqual(TEXT("Negative step magnitude is treated as a magnitude"), SisaAngle::StepTowards(0.0, 90.0, -5.0), 5.0, 1e-9);
		});
	});

	Describe("StepTowardsLinear", [this]()
	{
		It("does not wrap, so elevation can stay negative", [this]()
		{
			TestEqual(TEXT("Down to -5"), SisaAngle::StepTowardsLinear(0.0, -5.0, 10.0), -5.0, 1e-9);
			TestEqual(TEXT("Limited"), SisaAngle::StepTowardsLinear(0.0, 45.0, 3.0), 3.0, 1e-9);
			TestEqual(TEXT("No overshoot"), SisaAngle::StepTowardsLinear(44.0, 45.0, 3.0), 45.0, 1e-9);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
