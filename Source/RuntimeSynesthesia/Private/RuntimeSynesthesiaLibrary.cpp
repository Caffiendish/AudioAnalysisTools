// Fill out your copyright notice in the Description page of Project Settings.


#include "RuntimeSynesthesiaLibrary.h"

#include "AudioAnalyzerNRTFacade.h"
#include "AudioAnalyzerModule.h"
#include "IAudioAnalyzerNRTInterface.h"
#include "Features/IModularFeatures.h"
#include "SampleBuffer.h"
#include <AudioAnalyzerFacade.h>
#include "AudioAnalyzerNRT.h"
#include "OnsetNRT.h"
#include "OnsetNRTFactory.h"

URuntimeSynesthesiaLibrary* URuntimeSynesthesiaLibrary::CreateRuntimeSynesthesia()
{
	return NewObject<URuntimeSynesthesiaLibrary>();
}

UOnsetNRT* URuntimeSynesthesiaLibrary::Analyse(USoundWave* soundWave)
{

	// Create result and worker from factory
	Audio::FAnalyzerNRTParameters AnalyzerNRTParameters(44100, soundWave->NumChannels);

	Audio::FOnsetNRTResult* Result = new Audio::FOnsetNRTResult();
	Audio::FOnsetNRTSettings* Settings = new Audio::FOnsetNRTSettings();
	Audio::FOnsetNRTWorker* Worker = new Audio::FOnsetNRTWorker(AnalyzerNRTParameters, *Settings);

	
	//////////////////////////////////////////////////////////////////////////
	// I redid these lines without actually loading the project, from memory
	auto buffer = soundWave->RawData.GetCopyAsBuffer(0,false);


	// Convert 16 bit pcm to 32 bit float
	Audio::TSampleBuffer<float> FloatSamples((const int16*)buffer.GetView().GetData(), buffer.BulkDataSize / 2, soundWave->NumChannels, 44100);
	// So they almost certainly don't work. But this is the gist of what they were, so I'm sure anyone who looks will understand :D
	//////////////////////////////////////////////////////////////////////////


	// Perform and finalize audio analysis.
	Worker->Analyze(FloatSamples.GetArrayView(), Result);
	Worker->Finalize(Result);

	UOnsetNRT* onset = NewObject<UOnsetNRT>();

	onset->Settings = TObjectPtr<UOnsetNRTSettings>();
	auto fres = UAudioAnalyzerNRT::FResultSharedPtr(Result);

	///////////////////// Only available in Editor ///////////////////////////
	// Needs a custom solution written
	onset->SetResult(fres);
	//////////////////////////////////////////////////////////////////////////

	auto ons = Result->GetOnsetsForChannel(0);

	for (int i = 0; i < ons.Num(); i++)
	{
		UE_LOG(LogRuntimeSynesthesia, Warning, TEXT("Strength: %f"), ons[i].Strength);
	}

	return onset;
}
