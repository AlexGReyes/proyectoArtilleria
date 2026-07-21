#include "UtmMgrsConverter.h"

namespace
{
	// Shared by GetLatitudeBandLetter and GetLatitudeBandRangeDegrees so the two can never disagree.
	const FString LatitudeBandLetters = TEXT("CDEFGHJKLMNPQRSTUVWX");
}

double FUtmMgrsConverter::GetEccentricitySquared()
{
	return Flattening * (2.0 - Flattening);
}

int32 FUtmMgrsConverter::GetZoneNumber(double LongitudeDegrees)
{
	double Lon = FMath::Fmod(LongitudeDegrees + 180.0, 360.0);
	if (Lon < 0.0)
	{
		Lon += 360.0;
	}
	const int32 Zone = FMath::FloorToInt(Lon / 6.0) + 1;
	return FMath::Clamp(Zone, 1, 60);
}

double FUtmMgrsConverter::GetCentralMeridian(int32 ZoneNumber)
{
	return (ZoneNumber - 1) * 6.0 - 180.0 + 3.0;
}

TCHAR FUtmMgrsConverter::GetLatitudeBandLetter(double LatitudeDegrees)
{
	if (LatitudeDegrees < -80.0 || LatitudeDegrees > 84.0)
	{
		return TEXT('Z');
	}
	int32 Index = FMath::FloorToInt((LatitudeDegrees + 80.0) / 8.0);
	Index = FMath::Clamp(Index, 0, LatitudeBandLetters.Len() - 1);
	return LatitudeBandLetters[Index];
}

bool FUtmMgrsConverter::GetLatitudeBandRangeDegrees(TCHAR Band, double& OutMinLat, double& OutMaxLat)
{
	int32 Index = INDEX_NONE;
	for (int32 i = 0; i < LatitudeBandLetters.Len(); ++i)
	{
		if (LatitudeBandLetters[i] == Band)
		{
			Index = i;
			break;
		}
	}
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutMinLat = Index * 8.0 - 80.0;
	OutMaxLat = (Index == LatitudeBandLetters.Len() - 1) ? 84.0 : (OutMinLat + 8.0);
	return true;
}

void FUtmMgrsConverter::GetGridSquareId(int32 ZoneNumber, double Easting, double Northing, TCHAR& OutColLetter, TCHAR& OutRowLetter)
{
	static const FString ColSetA = TEXT("ABCDEFGH"); // zone % 3 == 1
	static const FString ColSetB = TEXT("JKLMNPQR"); // zone % 3 == 2
	static const FString ColSetC = TEXT("STUVWXYZ"); // zone % 3 == 0
	const FString& ColLetters = (ZoneNumber % 3 == 1) ? ColSetA : (ZoneNumber % 3 == 2) ? ColSetB : ColSetC;

	int32 ColIndex = FMath::FloorToInt(Easting / 100000.0) - 1;
	ColIndex = ((ColIndex % ColLetters.Len()) + ColLetters.Len()) % ColLetters.Len();
	OutColLetter = ColLetters[ColIndex];

	static const FString RowSetOdd = TEXT("ABCDEFGHJKLMNPQRSTUV");
	static const FString RowSetEven = TEXT("FGHJKLMNPQRSTUVABCDE");
	const FString& RowLetters = (ZoneNumber % 2 != 0) ? RowSetOdd : RowSetEven;

	int32 RowIndex = FMath::FloorToInt(Northing / 100000.0);
	RowIndex = ((RowIndex % RowLetters.Len()) + RowLetters.Len()) % RowLetters.Len();
	OutRowLetter = RowLetters[RowIndex];
}

bool FUtmMgrsConverter::DecodeGridSquareId(int32 ZoneNumber, TCHAR ColLetter, TCHAR RowLetter, double& OutEasting100kBase, double& OutNorthing100kBase)
{
	static const FString ColSetA = TEXT("ABCDEFGH");
	static const FString ColSetB = TEXT("JKLMNPQR");
	static const FString ColSetC = TEXT("STUVWXYZ");
	const FString& ColLetters = (ZoneNumber % 3 == 1) ? ColSetA : (ZoneNumber % 3 == 2) ? ColSetB : ColSetC;

	int32 ColIndex = INDEX_NONE;
	for (int32 i = 0; i < ColLetters.Len(); ++i)
	{
		if (ColLetters[i] == ColLetter)
		{
			ColIndex = i;
			break;
		}
	}
	if (ColIndex == INDEX_NONE)
	{
		return false;
	}
	OutEasting100kBase = (ColIndex + 1) * 100000.0;

	static const FString RowSetOdd = TEXT("ABCDEFGHJKLMNPQRSTUV");
	static const FString RowSetEven = TEXT("FGHJKLMNPQRSTUVABCDE");
	const FString& RowLetters = (ZoneNumber % 2 != 0) ? RowSetOdd : RowSetEven;

	int32 RowIndex = INDEX_NONE;
	for (int32 i = 0; i < RowLetters.Len(); ++i)
	{
		if (RowLetters[i] == RowLetter)
		{
			RowIndex = i;
			break;
		}
	}
	if (RowIndex == INDEX_NONE)
	{
		return false;
	}
	OutNorthing100kBase = RowIndex * 100000.0;

	return true;
}

TSisaResult<FUtmCoordinate> FUtmMgrsConverter::ToUtm(const FGeoCoordinate& Geo)
{
	if (Geo.Latitude < -80.0 || Geo.Latitude > 84.0)
	{
		return TSisaResult<FUtmCoordinate>::Err(
			TEXT("GIS.Utm.OutOfRange"),
			TEXT("UTM is only defined for latitudes in [-80, 84]; polar (UPS) coordinates are not supported."));
	}

	const double E2 = GetEccentricitySquared();
	const double EPrime2 = E2 / (1.0 - E2);

	const int32 ZoneNumber = GetZoneNumber(Geo.Longitude);
	const double LonOrigin = GetCentralMeridian(ZoneNumber);

	const double LatRad = FMath::DegreesToRadians(Geo.Latitude);
	const double LonRad = FMath::DegreesToRadians(Geo.Longitude);
	const double LonOriginRad = FMath::DegreesToRadians(LonOrigin);

	const double SinLat = FMath::Sin(LatRad);
	const double CosLat = FMath::Cos(LatRad);
	const double TanLat = FMath::Tan(LatRad);

	const double N = SemiMajorAxis / FMath::Sqrt(1.0 - E2 * SinLat * SinLat);
	const double T = TanLat * TanLat;
	const double C = EPrime2 * CosLat * CosLat;
	const double A = (LonRad - LonOriginRad) * CosLat;

	const double M = SemiMajorAxis * (
		(1.0 - E2 / 4.0 - 3.0 * E2 * E2 / 64.0 - 5.0 * E2 * E2 * E2 / 256.0) * LatRad
		- (3.0 * E2 / 8.0 + 3.0 * E2 * E2 / 32.0 + 45.0 * E2 * E2 * E2 / 1024.0) * FMath::Sin(2.0 * LatRad)
		+ (15.0 * E2 * E2 / 256.0 + 45.0 * E2 * E2 * E2 / 1024.0) * FMath::Sin(4.0 * LatRad)
		- (35.0 * E2 * E2 * E2 / 3072.0) * FMath::Sin(6.0 * LatRad));

	const double Easting = ScaleFactor * N * (A + (1.0 - T + C) * FMath::Pow(A, 3) / 6.0
		+ (5.0 - 18.0 * T + T * T + 72.0 * C - 58.0 * EPrime2) * FMath::Pow(A, 5) / 120.0) + 500000.0;

	double Northing = ScaleFactor * (M + N * TanLat * (A * A / 2.0
		+ (5.0 - T + 9.0 * C + 4.0 * C * C) * FMath::Pow(A, 4) / 24.0
		+ (61.0 - 58.0 * T + T * T + 600.0 * C - 330.0 * EPrime2) * FMath::Pow(A, 6) / 720.0));

	const bool bNorthern = Geo.Latitude >= 0.0;
	if (!bNorthern)
	{
		Northing += 10000000.0;
	}

	FUtmCoordinate Utm;
	Utm.ZoneNumber = ZoneNumber;
	Utm.bNorthernHemisphere = bNorthern;
	Utm.Easting = Easting;
	Utm.Northing = Northing;
	Utm.HeightMeters = Geo.HeightMeters;
	return TSisaResult<FUtmCoordinate>::Ok(Utm);
}

TSisaResult<FGeoCoordinate> FUtmMgrsConverter::FromUtm(const FUtmCoordinate& Utm)
{
	if (Utm.ZoneNumber < 1 || Utm.ZoneNumber > 60)
	{
		return TSisaResult<FGeoCoordinate>::Err(TEXT("GIS.Utm.InvalidZone"), TEXT("UTM zone number must be in [1, 60]."));
	}

	const double E2 = GetEccentricitySquared();
	const double EPrime2 = E2 / (1.0 - E2);
	const double E1 = (1.0 - FMath::Sqrt(1.0 - E2)) / (1.0 + FMath::Sqrt(1.0 - E2));

	double Northing = Utm.Northing;
	if (!Utm.bNorthernHemisphere)
	{
		Northing -= 10000000.0;
	}

	const double M = Northing / ScaleFactor;
	const double Mu = M / (SemiMajorAxis * (1.0 - E2 / 4.0 - 3.0 * E2 * E2 / 64.0 - 5.0 * E2 * E2 * E2 / 256.0));

	const double Phi1 = Mu
		+ (3.0 * E1 / 2.0 - 27.0 * E1 * E1 * E1 / 32.0) * FMath::Sin(2.0 * Mu)
		+ (21.0 * E1 * E1 / 16.0 - 55.0 * E1 * E1 * E1 * E1 / 32.0) * FMath::Sin(4.0 * Mu)
		+ (151.0 * E1 * E1 * E1 / 96.0) * FMath::Sin(6.0 * Mu)
		+ (1097.0 * E1 * E1 * E1 * E1 / 512.0) * FMath::Sin(8.0 * Mu);

	const double SinPhi1 = FMath::Sin(Phi1);
	const double CosPhi1 = FMath::Cos(Phi1);
	const double TanPhi1 = FMath::Tan(Phi1);

	const double C1 = EPrime2 * CosPhi1 * CosPhi1;
	const double T1 = TanPhi1 * TanPhi1;
	const double N1 = SemiMajorAxis / FMath::Sqrt(1.0 - E2 * SinPhi1 * SinPhi1);
	const double R1 = SemiMajorAxis * (1.0 - E2) / FMath::Pow(1.0 - E2 * SinPhi1 * SinPhi1, 1.5);
	const double D = (Utm.Easting - 500000.0) / (N1 * ScaleFactor);

	const double LatRad = Phi1 - (N1 * TanPhi1 / R1) * (D * D / 2.0
		- (5.0 + 3.0 * T1 + 10.0 * C1 - 4.0 * C1 * C1 - 9.0 * EPrime2) * FMath::Pow(D, 4) / 24.0
		+ (61.0 + 90.0 * T1 + 298.0 * C1 + 45.0 * T1 * T1 - 252.0 * EPrime2 - 3.0 * C1 * C1) * FMath::Pow(D, 6) / 720.0);

	double LonRad = (D - (1.0 + 2.0 * T1 + C1) * FMath::Pow(D, 3) / 6.0
		+ (5.0 - 2.0 * C1 + 28.0 * T1 - 3.0 * C1 * C1 + 8.0 * EPrime2 + 24.0 * T1 * T1) * FMath::Pow(D, 5) / 120.0) / CosPhi1;

	const double LonOriginRad = FMath::DegreesToRadians(GetCentralMeridian(Utm.ZoneNumber));
	LonRad += LonOriginRad;

	FGeoCoordinate Geo;
	Geo.Latitude = FMath::RadiansToDegrees(LatRad);
	Geo.Longitude = FMath::RadiansToDegrees(LonRad);
	Geo.HeightMeters = Utm.HeightMeters;
	return TSisaResult<FGeoCoordinate>::Ok(Geo);
}

TSisaResult<FString> FUtmMgrsConverter::ToMgrs(const FGeoCoordinate& Geo, int32 PrecisionDigits)
{
	PrecisionDigits = FMath::Clamp(PrecisionDigits, 1, 5);

	const TSisaResult<FUtmCoordinate> UtmResult = ToUtm(Geo);
	if (UtmResult.IsError())
	{
		return TSisaResult<FString>::Err(UtmResult.GetError());
	}
	const FUtmCoordinate& Utm = UtmResult.GetValue();

	const TCHAR Band = GetLatitudeBandLetter(Geo.Latitude);
	if (Band == TEXT('Z'))
	{
		return TSisaResult<FString>::Err(TEXT("GIS.Mgrs.OutOfRange"), TEXT("Latitude outside the supported MGRS band range [-80, 84]."));
	}

	TCHAR ColLetter, RowLetter;
	GetGridSquareId(Utm.ZoneNumber, Utm.Easting, Utm.Northing, ColLetter, RowLetter);

	const double DivisorPow = FMath::Pow(10.0, 5 - PrecisionDigits);
	const int32 EastingDigits = FMath::RoundToInt(FMath::Fmod(Utm.Easting, 100000.0) / DivisorPow);
	const int32 NorthingDigits = FMath::RoundToInt(FMath::Fmod(Utm.Northing, 100000.0) / DivisorPow);

	const FString Mgrs = FString::Printf(TEXT("%02d%c%c%c%0*d%0*d"),
		Utm.ZoneNumber, Band, ColLetter, RowLetter,
		PrecisionDigits, EastingDigits,
		PrecisionDigits, NorthingDigits);

	return TSisaResult<FString>::Ok(Mgrs);
}

TSisaResult<FGeoCoordinate> FUtmMgrsConverter::FromMgrs(const FString& Mgrs)
{
	FString Trimmed = Mgrs.TrimStartAndEnd().ToUpper();
	Trimmed.ReplaceInline(TEXT(" "), TEXT(""));

	int32 Index = 0;
	while (Index < Trimmed.Len() && Index < 2 && FChar::IsDigit(Trimmed[Index]))
	{
		++Index;
	}
	if (Index == 0)
	{
		return TSisaResult<FGeoCoordinate>::Err(TEXT("GIS.Mgrs.ParseError"), TEXT("Missing UTM zone number."));
	}
	const int32 ZoneNumber = FCString::Atoi(*Trimmed.Left(Index));
	if (ZoneNumber < 1 || ZoneNumber > 60)
	{
		return TSisaResult<FGeoCoordinate>::Err(TEXT("GIS.Mgrs.ParseError"), TEXT("UTM zone number out of range."));
	}

	if (Index + 3 > Trimmed.Len())
	{
		return TSisaResult<FGeoCoordinate>::Err(TEXT("GIS.Mgrs.ParseError"), TEXT("String too short to contain the band and 100km grid square letters."));
	}
	const TCHAR Band = Trimmed[Index];
	const TCHAR ColLetter = Trimmed[Index + 1];
	const TCHAR RowLetter = Trimmed[Index + 2];
	Index += 3;

	double MinLat, MaxLat;
	if (!GetLatitudeBandRangeDegrees(Band, MinLat, MaxLat))
	{
		return TSisaResult<FGeoCoordinate>::Err(TEXT("GIS.Mgrs.ParseError"), TEXT("Unrecognized latitude band letter."));
	}

	double Easting100kBase, Northing100kBase;
	if (!DecodeGridSquareId(ZoneNumber, ColLetter, RowLetter, Easting100kBase, Northing100kBase))
	{
		return TSisaResult<FGeoCoordinate>::Err(TEXT("GIS.Mgrs.ParseError"), TEXT("Unrecognized 100km grid square letters for this zone."));
	}

	const FString DigitsPart = Trimmed.Mid(Index);
	if (DigitsPart.Len() == 0 || DigitsPart.Len() % 2 != 0 || DigitsPart.Len() > 10)
	{
		return TSisaResult<FGeoCoordinate>::Err(
			TEXT("GIS.Mgrs.ParseError"),
			TEXT("Easting/Northing digit block must be a non-empty, even-length string of at most 10 digits."));
	}
	const int32 PrecisionDigits = DigitsPart.Len() / 2;
	const FString EastingDigitsStr = DigitsPart.Left(PrecisionDigits);
	const FString NorthingDigitsStr = DigitsPart.Mid(PrecisionDigits);
	for (const FString& Part : { EastingDigitsStr, NorthingDigitsStr })
	{
		for (TCHAR C : Part)
		{
			if (!FChar::IsDigit(C))
			{
				return TSisaResult<FGeoCoordinate>::Err(TEXT("GIS.Mgrs.ParseError"), TEXT("Non-digit character in easting/northing block."));
			}
		}
	}

	const double DivisorPow = FMath::Pow(10.0, 5 - PrecisionDigits);
	const double EastingOffset = FCString::Atod(*EastingDigitsStr) * DivisorPow;
	const double NorthingOffset = FCString::Atod(*NorthingDigitsStr) * DivisorPow;
	const double Easting = Easting100kBase + EastingOffset;

	// The row-letter sequence repeats every 2,000,000m and is continuous across the
	// equator, so the same digits can map to several candidate northings; the
	// latitude band letter (already parsed above) is what disambiguates which one
	// is correct. Bounded brute-force search, see the class comment in the header.
	FUtmCoordinate Candidate;
	Candidate.ZoneNumber = ZoneNumber;
	Candidate.Easting = Easting;

	for (int32 K = 0; K < 6; ++K)
	{
		const double CandidateNorthing = Northing100kBase + NorthingOffset + K * 2000000.0;
		if (CandidateNorthing >= 10000000.0)
		{
			continue;
		}

		for (bool bNorthernGuess : { true, false })
		{
			Candidate.bNorthernHemisphere = bNorthernGuess;
			Candidate.Northing = CandidateNorthing;

			const TSisaResult<FGeoCoordinate> GeoResult = FromUtm(Candidate);
			if (GeoResult.IsOk() && GeoResult.GetValue().Latitude >= MinLat && GeoResult.GetValue().Latitude <= MaxLat)
			{
				return GeoResult;
			}
		}
	}

	return TSisaResult<FGeoCoordinate>::Err(
		TEXT("GIS.Mgrs.Ambiguous"),
		TEXT("Could not resolve a unique latitude band for this MGRS grid square; the string may be malformed."));
}
