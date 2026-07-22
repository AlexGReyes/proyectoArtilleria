#pragma once

#include "CoreMinimal.h"

/**
 * Compass/mount angle math shared across SISA plugins: Artillery lays a real
 * piece with it, Hardware slews its simulated mount with it. Header-only free
 * functions — no UObject, no reflection, nothing to link — so any module can
 * use them without a dependency beyond SISACore.
 *
 * "Compass" convention throughout: degrees clockwise from true North.
 */
namespace SisaAngle
{
	/** Wraps an angle into [0, 360). */
	inline double Normalize360(double AngleDegrees)
	{
		const double Wrapped = FMath::Fmod(AngleDegrees, 360.0);
		return Wrapped < 0.0 ? Wrapped + 360.0 : Wrapped;
	}

	/** Signed shortest angular difference To - From, in (-180, 180]. */
	inline double ShortestDelta(double FromDegrees, double ToDegrees)
	{
		double Delta = Normalize360(ToDegrees) - Normalize360(FromDegrees);
		if (Delta > 180.0)
		{
			Delta -= 360.0;
		}
		else if (Delta <= -180.0)
		{
			Delta += 360.0;
		}
		return Delta;
	}

	/**
	 * Moves a compass angle toward a target by at most MaxStepDegrees, taking
	 * the shortest way around and never overshooting. The result is normalized.
	 */
	inline double StepTowards(double CurrentDegrees, double TargetDegrees, double MaxStepDegrees)
	{
		const double Delta = ShortestDelta(CurrentDegrees, TargetDegrees);
		const double Step = FMath::Clamp(Delta, -FMath::Abs(MaxStepDegrees), FMath::Abs(MaxStepDegrees));
		return Normalize360(CurrentDegrees + Step);
	}

	/**
	 * Moves a linear (non-wrapping) angle such as elevation toward a target by
	 * at most MaxStepDegrees, without overshooting.
	 */
	inline double StepTowardsLinear(double CurrentDegrees, double TargetDegrees, double MaxStepDegrees)
	{
		const double Delta = TargetDegrees - CurrentDegrees;
		return CurrentDegrees + FMath::Clamp(Delta, -FMath::Abs(MaxStepDegrees), FMath::Abs(MaxStepDegrees));
	}
}
