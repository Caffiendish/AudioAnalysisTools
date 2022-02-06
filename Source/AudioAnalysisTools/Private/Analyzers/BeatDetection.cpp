// Georgy Treshchev 2022.

#include "Analyzers/BeatDetection.h"
#include "AudioAnalysisToolsDefines.h"
#include "Math/UnrealMathUtility.h"

UBeatDetection::UBeatDetection()
	: HistoryPosition(0),
	  FFTSubbandsSize(0),
	  EnergyHistorySize(0)
{
}

UBeatDetection* UBeatDetection::CreateBeatDetection(int32 InFFTSubbandsSize, int32 InEnergyHistorySize)
{
	UBeatDetection* BeatDetection{NewObject<UBeatDetection>()};
	BeatDetection->UpdateFFTSubbandsSize(InFFTSubbandsSize);
	BeatDetection->UpdateEnergyHistorySize(InEnergyHistorySize);
	return BeatDetection;
}

void UBeatDetection::UpdateFFTSubbandsSize(int32 InFFTSubbandsSize)
{
	UE_LOG(LogAudioAnalysis, Log, TEXT("Updating Beat Detection FFT subbands size from '%d' to '%d'"), FFTSubbandsSize, InFFTSubbandsSize);
	
	FFTSubbandsSize = InFFTSubbandsSize;

	FFTSubbands.SetNum(FFTSubbandsSize);
	FFTAverageEnergy.SetNum(FFTSubbandsSize);
	FFTVariance.SetNum(FFTSubbandsSize);
	FFTBeatValues.SetNum(FFTSubbandsSize);
	EnergyHistory.SetNum(FFTSubbandsSize);
}

void UBeatDetection::UpdateEnergyHistorySize(int32 InEnergyHistorySize)
{
	UE_LOG(LogAudioAnalysis, Log, TEXT("Updating Beat Detection energy history size from '%d' to '%d'"), EnergyHistorySize, InEnergyHistorySize);
	
	EnergyHistorySize = InEnergyHistorySize;

	for (int32 SubbandIndex = 0; SubbandIndex < FFTSubbandsSize; ++SubbandIndex)
	{
		EnergyHistory[SubbandIndex].SetNum(EnergyHistorySize);
	}
}

void UBeatDetection::UpdateFFT(const TArray<float>& MagnitudeSpectrum)
{
	const int32 MagnitudeSpectrumSize{MagnitudeSpectrum.Num()};

	/** Sub-band calculation */
	for (int32 SubbandIndex = 0; SubbandIndex < FFTSubbandsSize; ++SubbandIndex)
	{
		FFTSubbands[SubbandIndex] = 0;

		for (int32 SubbandInternalIndex = 0; SubbandInternalIndex < MagnitudeSpectrumSize / FFTSubbandsSize; ++SubbandInternalIndex)
		{
			FFTSubbands[SubbandIndex] += MagnitudeSpectrum[SubbandIndex * (MagnitudeSpectrumSize / FFTSubbandsSize) + SubbandInternalIndex];
		}
		/** After summing the subband values, divide the added number of times to get the average value */
		FFTSubbands[SubbandIndex] *= static_cast<float>(FFTSubbandsSize) / MagnitudeSpectrumSize;

		/** Calculation of subband variance value */
		for (int32 SubbandInternalIndex = 0; SubbandInternalIndex < MagnitudeSpectrumSize / FFTSubbandsSize; ++SubbandInternalIndex)
		{
			FFTVariance[SubbandIndex] += FMath::Pow(MagnitudeSpectrum[SubbandIndex * (MagnitudeSpectrumSize / FFTSubbandsSize) + SubbandInternalIndex] - FFTSubbands[SubbandIndex], 2);
		}
		FFTVariance[SubbandIndex] *= static_cast<float>(FFTSubbandsSize) / MagnitudeSpectrumSize;

		/** Reduce possible noise with linear digression using some magic numbers */
		FFTBeatValues[SubbandIndex] = (-0.0025714 * FFTVariance[SubbandIndex]) + 1.15142857;
	}

	/** Calculation of energy average */
	for (int32 SubbandIndex = 0; SubbandIndex < FFTSubbandsSize; ++SubbandIndex)
	{
		FFTAverageEnergy[SubbandIndex] = 0;
		for (int32 EnergyHistoryIndex = 0; EnergyHistoryIndex < EnergyHistorySize; ++EnergyHistoryIndex)
		{
			/** Average of total energy += Energy history of each subband */
			FFTAverageEnergy[SubbandIndex] += EnergyHistory[SubbandIndex][EnergyHistoryIndex];
		}

		/** Divide the sum by the history energy to get a weighted average */
		FFTAverageEnergy[SubbandIndex] /= EnergyHistorySize;
	}

	/** Put new values into the energy history */
	for (int32 SubbandIndex = 0; SubbandIndex < FFTSubbandsSize; ++SubbandIndex)
	{
		/** Add the calculated subband to the HistoryPosition in the energy history */
		EnergyHistory[SubbandIndex][HistoryPosition] = FFTSubbands[SubbandIndex];
	}

	/** A pseudo-cyclic list is represented by circular array indexes */
	HistoryPosition = (HistoryPosition + 1) % EnergyHistorySize;
}

void UBeatDetection::ProcessMagnitude(const TArray<float>& MagnitudeSpectrum)
{
	UpdateFFT(MagnitudeSpectrum);
}

bool UBeatDetection::IsBeat(int32 SubBand) const
{
	return FFTSubbands[SubBand] > FFTAverageEnergy[SubBand] * FFTBeatValues[SubBand];
}

bool UBeatDetection::IsKick() const
{
	return IsBeat(KICK_BAND);
}

bool UBeatDetection::IsSnare() const
{
	constexpr int32 Low{1};
	const int32 High{FFTSubbandsSize / 3};
	const int32 Threshold{(High - Low) / 3};

	return IsBeatRange(Low, High, Threshold);
}

bool UBeatDetection::IsHiHat() const
{
	const int32 Low{FFTSubbandsSize / 2};
	const int32 High{FFTSubbandsSize - 1};
	const int32 Threshold{(High - Low) / 3};

	return IsBeatRange(Low, High, Threshold);
}

bool UBeatDetection::IsBeatRange(int32 Low, int32 High, int32 Threshold) const
{
	if (!(Low > 0 && Low < FFTSubbandsSize))
	{
		UE_LOG(LogAudioAnalysis, Error, TEXT("Cannot check if beat is in range: low subband is '%d', expected > '0' and < '%d'"), Low, FFTSubbandsSize);
		return false;
	}

	if (!(High > 0 && High < FFTSubbandsSize))
	{
		UE_LOG(LogAudioAnalysis, Error, TEXT("Cannot check if beat is in range: high subband is '%d', expected > '0', < '%d'"), High, FFTSubbandsSize);
		return false;
	}

	if (!(High > Low))
	{
		UE_LOG(LogAudioAnalysis, Error, TEXT("Cannot check if beat is in range: high subband ('%d') must be more than low subband ('%d')"), High, Low);
		return false;
	}

	int32 NumOfBeats = 0;

	for (int32 Index = Low; Index < High + 1; ++Index)
	{
		if (IsBeat(Index))
		{
			NumOfBeats++;
		}
	}

	return NumOfBeats > Threshold;
}

float UBeatDetection::GetBand(int32 Subband) const
{
	if (!(Subband > 0 && Subband < FFTSubbandsSize))
	{
		UE_LOG(LogAudioAnalysis, Error, TEXT("Cannot get FFT subband: specified subband is '%d', expected > '0' and < '%d'"), Subband, FFTSubbandsSize);
		return -1;
	}
	return FFTSubbands[Subband];
}