/////////////////////////////////////////////////////////////////////////////
// EAPatchClient/BuildPatch.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientApp/BuildPatch.h>
#include <EAPatchClient/PatchDirectory.h>   // Needed only if you are using the PatchDirectory system.
#include <EAPatchClient/PatchBuilder.h>     // Always needed for patch building.
#include <EAPatchClient/Telemetry.h>        // Needed only if you register to listen for telemetry events.
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
// extern
///////////////////////////////////////////////////////////////////////////////

extern bool gbTraceDirectoryRetrieverEvents;
extern bool gbTraceDirectoryRetrieverTelemetry;
extern bool gbTracePatchBuilderEvents;
extern bool gbTracePatchBuilderTelemetry;
extern bool ShouldStop();


///////////////////////////////////////////////////////////////////////////////
// ExamplePatchBuilderEventCallback
///////////////////////////////////////////////////////////////////////////////

class ExamplePatchBuilderEventCallback : public EA::Patch::PatchBuilderEventCallback
{
public:
    void EAPatchStart(EA::Patch::PatchBuilder*, intptr_t)
        { printf("Patch started\n"); }

    void EAPatchProgress(EA::Patch::PatchBuilder*, intptr_t, double patchProgress)
        { printf("Progress: %2.0f%%\n", patchProgress * 100); }

    void EAPatchRetryableNetworkError(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pURL)
        { printf("Network re-tryable error: %s\n", pURL); }

    void EAPatchNetworkAvailability(EA::Patch::PatchBuilder*, intptr_t, bool available)
        { printf("Network availability change: now %s\n", available ? "available" : "unavailable"); }

    void EAPatchError(EA::Patch::PatchBuilder*, intptr_t, EA::Patch::Error& error)
        { printf("Error: %s\n", error.GetErrorString()); }

    void EAPatchNewState(EA::Patch::PatchBuilder*, intptr_t, int newState)
        { printf("State: %s\n", EA::Patch::PatchBuilder::GetStateString((EA::Patch::PatchBuilder::State)newState)); }

    void EAPatchBeginFileDownload(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pFilePath, const char8_t* pFileURL)
        { printf("Begin file download:\n   %s ->\n   %s\n", pFilePath, pFileURL); }

    void EAPatchEndFileDownload(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pFilePath, const char8_t* pFileURL)
        { printf("End file download:\n   %s->\n   %s\n", pFilePath, pFileURL); }

    void EAPatchRenameFile(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pPrevFilePath, const char8_t* pNewFilePath)
        { printf("Renaming file:\n   %s->\n   %s\n", pPrevFilePath, pNewFilePath); }

    void EAPatchDeleteFile(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pFilePath)
        { printf("Deleting file:\n   %s\n", pFilePath); }

    void EAPatchRenameDirectory(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pPrevDirPath, const char8_t* pNewDirPath)
        { printf("Renaming directory:\n   %s->\n   %s\n", pPrevDirPath, pNewDirPath); }

    void EAPatchDeleteDirectory(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pDirPath)
        { printf("Deleting directory:\n   %s\n", pDirPath); }

    void EAPatchStop(EA::Patch::PatchBuilder*, intptr_t, EA::Patch::AsyncStatus status)
        { printf("Patch stopped with state %s\n", GetAsyncStatusString(status)); }

}; // class ExamplePatchBuilderEventCallback



///////////////////////////////////////////////////////////////////////////////
// ExampleTelemetryEventCallback
///////////////////////////////////////////////////////////////////////////////

class ExampleTelemetryEventCallback : public EA::Patch::TelemetryEventCallback
{
public:
    void TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchSystemInit& tpsi)
    {
        printf("TelemetryPatchSystemInit:\n");
        printf("   Date: %s\n", tpsi.mDate.c_str());
    }

    void TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchSystemShutdown& tpss)
    {
        printf("TelemetryPatchSystemShutdown:\n");
        printf("   Date: %s\n", tpss.mDate.c_str());
    }

    void TelemetryEvent(intptr_t, EA::Patch::TelemetryDirectoryRetrievalBegin& tdrb)
    {
        printf("TelemetryDirectoryRetrievalBegin:\n");
        printf("   Date: %s\n", tdrb.mDate.c_str());
        printf("   URL:  %s\n", tdrb.mPatchDirectoryURL.c_str());
    }

    void TelemetryEvent(intptr_t, EA::Patch::TelemetryDirectoryRetrievalEnd& tdre)
    {
        printf("TelemetryDirectoryRetrievalEnd:\n");
        printf("   Date:              %s\n", tdre.mDate.c_str());
        printf("   URL:               %s\n", tdre.mPatchDirectoryURL.c_str());
        printf("   Status:            %s\n", tdre.mStatus.c_str());
        printf("   Dir download vol:  %s\n", tdre.mDirDownloadVolume.c_str());
        printf("   Info download vol: %s\n", tdre.mInfoDownloadVolume.c_str());
    }

    void TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchBuildBegin& tpbb)
    {
        printf("TelemetryPatchBuildBegin:\n");
        printf("   Date:   %s\n", tpbb.mDate.c_str());
        printf("   URL:    %s\n", tpbb.mPatchImplURL.c_str());
        printf("   Id:     %s\n", tpbb.mPatchId.c_str());
        printf("   Course: %s\n", tpbb.mPatchCourse.c_str());
    }

    void TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchBuildEnd& tpbe)
    {
        printf("TelemetryPatchBuildEnd:\n");
        printf("   Date:                    %s\n", tpbe.mDate.c_str());
        printf("   URL:                     %s\n", tpbe.mPatchImplURL.c_str());
        printf("   Id:                      %s\n", tpbe.mPatchId.c_str());
        printf("   Course:                  %s\n", tpbe.mPatchCourse.c_str());
        printf("   Direction:               %s\n", tpbe.mPatchDirection.c_str());
        printf("   Status:                  %s\n", tpbe.mStatus.c_str());
        printf("   HTTP Bps:                %s\n", tpbe.mDownloadSpeedBytesPerSecond.c_str());
        printf("   Disk Bps:                %s\n", tpbe.mDiskSpeedBytesPerSecond.c_str());
        printf("   Impl download vol:       %s\n", tpbe.mImplDownloadVolume.c_str());
        printf("   Diff download vol:       %s\n", tpbe.mDiffDownloadVolume.c_str());
        printf("   File download vol:       %s\n", tpbe.mFileDownloadVolume.c_str());
        printf("   File download vol total: %s\n", tpbe.mFileDownloadVolumeFinal.c_str());
        printf("   File copy vol:           %s\n", tpbe.mFileCopyVolume.c_str());
        printf("   File copy vol total:     %s\n", tpbe.mFileCopyVolumeFinal.c_str());
        printf("   Time estimate:           %s\n", tpbe.mPatchTimeEstimate.c_str());
        printf("   Time actual:             %s\n", tpbe.mPatchTime.c_str());
    }

    void TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchCancelBegin& tpcb)
    {
        printf("TelemetryPatchCancelBegin:\n");
        printf("   Date: %s\n", tpcb.mDate.c_str());
        printf("   URL:  %s\n", tpcb.mPatchImplURL.c_str());
        printf("   Id:   %s\n", tpcb.mPatchId.c_str());
    }

    void TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchCancelEnd& tpce)
    {
        printf("TelemetryPatchCancelEnd:\n");
        printf("   Date:          %s\n", tpce.mDate.c_str());
        printf("   URL:           %s\n", tpce.mPatchImplURL.c_str());
        printf("   Id:            %s\n", tpce.mPatchId.c_str());
        printf("   Direction:     %s\n", tpce.mPatchDirection.c_str());
        printf("   Status:        %s\n", tpce.mStatus.c_str());
        printf("   Time estimate: %s\n", tpce.mCancelTimeEstimate.c_str());
        printf("   Time actual:   %s\n", tpce.mCancelTime.c_str());
    }

}; // class ExampleTelemetryEventCallback


// This is the main patch building function, and is the code you want to look to in order
// to know how to integrate patching into your app. Some of the functionality below is 
// optional but useful for demonstration. A minimal example of patching code is thus:
//     
//     EA::Patch::SetAllocator(pCoreAllocator);
//     EA::Patch::PatchBuilder builder;
//     builder.SetDirPaths(pPatchDestinationDirPath, pPatchLocalDirPath);
//     builder.SetPatchInfo(patchInfo); // Or call the other version of this function.
//     builder.BuildPatchBlocking();
// 
static bool BuildPatch(const EA::Patch::PatchInfo* pPatchInfo, const char8_t* pPatchImplURL, 
                       const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath)
{
    // Need to set some ICoreAllocator. All EAPatch memory allocation uses this allocator, 
    // though DirtySDK (which EAPatch uses) has its own memory allocation scheme.
    EA::Patch::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());

    EA::Patch::PatchBuilder          builder;                   // This class is always needed to implement a patch.
    ExamplePatchBuilderEventCallback builderEventCallback;      // Optional.
    ExampleTelemetryEventCallback    builderTelemetryCallback;  // Optional.

    if(gbTracePatchBuilderEvents)
        builder.RegisterPatchBuilderEventCallback(&builderEventCallback, 0);

    if(gbTracePatchBuilderTelemetry)   
        builder.RegisterTelemetryEventCallback(&builderTelemetryCallback, 0);

    builder.SetDirPaths(pPatchDestinationDirPath, pPatchLocalDirPath);

    if(pPatchInfo)
        builder.SetPatchInfo(*pPatchInfo);
    else if(pPatchImplURL)
        builder.SetPatchInfo("ignore", pPatchImplURL); // To do: Make it so SetPatchInfo doesn't require a patch id first parameter.

    if(pPatchInfo || pPatchImplURL) // If the caller requested a patch build (as opposed to a cancel)...
    {
        if(builder.BuildPatchAsync()) // If we could begin the asynchronous patch build (this will always be true unless the system has a serious problem)...
        {
            while(!EA::Patch::IsAsyncOperationEnded(builder.GetAsyncStatus())) // While the patch isn't yet complete...
            {
                EA::Thread::ThreadSleep(100);

                if(ShouldStop()) // Call an external user function to test if the escape key or some other cancelling input was enacted.
                {
                    bool bPatchStopped;
                    builder.Stop(true, bPatchStopped);

                    if(bPatchStopped) // 99.9% of the time this will be true. It will be false only if Stop was called right as the patch was finishing.
                        printf("Patch was stopped due to user request.\n");
                    else
                        printf("Patch completed before user-requested stop could be executed.\n");

                    break;
                }
            }
        }
        else
            printf("Failed to start async patch build.\n");
    }
    else
    {
        bool bPatchCancelled;
        builder.Cancel(false, bPatchCancelled);

        if(bPatchCancelled) // 99.9% of the time this will be true. It will be false only if Stop was called right as the patch was finishing.
            printf("Patch was cancelled due to user request.\n");
        else
            printf("Patch completed before user-requested cancel could be executed.\n");
    }

    // We have a problem. builder.GetAsyncStatus may return that it's completed, but it still 
    // is executing some final code (e.g. notifying its callbacks). We need to fix this in 
    // either the builder and/or the builder API, but in the meantime we just put a sleep here
    // to give it time to exit.
    EA::Thread::ThreadSleep(500);

    // The status is how you tell if a build or cacnel completed successfully. See enum AsyncStatus to know more.
    EA::Patch::AsyncStatus status = builder.GetAsyncStatus();
    printf("Patch completed with status: %s\n", EA::Patch::GetAsyncStatusString(status));

    // You have to either call builder.HasError()/builder.GetError() or you have to 
    // call builder.EnableErrorAssertions(false). Otherwise PatchBuilder will assert
    // in its destructor that you didn't check its error state.
    if(builder.HasError())
    {
        EA::Patch::String sError;
        EA::Patch::Error  error = builder.GetError();
        error.GetErrorString(sError);
        printf("PatchBuilder error: %s\n", sError.c_str());
    }

    return (status == EA::Patch::kAsyncStatusCompleted);
}





// Build a patch from a .eaPatchInfo URL.
// This is an optional mode that an application is not likely to use.
// In this case we are doing something that's probably unusual: building a patch directly from a .eaPatchInfo file URL.
// This is unusual because more commonly an application will download a set of .eaPatchInfo files through DirectoryRetriever
// as part of presenting to the user a set of available patches. Also, if an application wants to directly download a patch
// instead of going through a patch directory, then the app would probably execute a patch from a .eaPatchImpl file URL.
// PatchBuilder knows how to work with PatchInfo struct and .eaPatchImpl URLs but currently not .eaPatchInfo URLs, simply
// because it's an uncommon thing. But we demonstrate manually downloading a .eaPatchInfo file here anyway.
#include <EAIO/EAStreamMemory.h>

bool BuildPatchFromInfoURL(const char8_t* pPatchInfoURL, const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath)
{
    EA::Patch::Error  error;
    EA::Patch::String sError;

    EA::IO::MemoryStream eaPatchInfoStream;
    eaPatchInfoStream.AddRef();
    eaPatchInfoStream.SetAllocator(EA::Patch::GetAllocator());
    eaPatchInfoStream.SetOption(EA::IO::MemoryStream::kOptionResizeEnabled, 1);

    EA::Patch::HTTP http;

    if(!http.GetFileBlocking(pPatchInfoURL, &eaPatchInfoStream))
    {
        error = http.GetError();
        error.GetErrorString(sError);
        printf("BuildPatchFromInfoURL error: %s\n", sError.c_str());
        return false;
    }

    EA::Patch::PatchInfoSerializer patchInfoSerializer;
    EA::Patch::PatchInfo           patchInfo;

    eaPatchInfoStream.SetPosition(0);
    patchInfoSerializer.Deserialize(patchInfo, &eaPatchInfoStream, true);

    if(patchInfoSerializer.HasError())
    {
        error = patchInfoSerializer.GetError();
        error.GetErrorString(sError);
        printf("BuildPatchFromInfoURL error: %s\n", sError.c_str());
        return false;
    }
    else if(patchInfo.HasError())
    {
        error = patchInfo.GetError();
        error.GetErrorString(sError);
        printf("BuildPatchFromInfoURL error: %s\n", sError.c_str());
        return false;
    }

    return BuildPatch(&patchInfo, NULL, pPatchDestinationDirPath, pPatchLocalDirPath);
}


// Build a patch from a .eaPatchInfo file on disk.
// This is an optional mode that an application is not likely to use, but we demonstrate it anyway.
bool BuildPatchFromInfoFile(const char8_t* pPatchInfoFilePath, const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath)
{
    EA::Patch::PatchInfoSerializer patchInfoSerializer;
    EA::Patch::PatchInfo           patchInfo;
    EA::Patch::Error               error;
    EA::Patch::String              sError;

    patchInfoSerializer.Deserialize(patchInfo, pPatchInfoFilePath, true);

    if(patchInfoSerializer.HasError())
    {
        error = patchInfoSerializer.GetError();
        error.GetErrorString(sError);
        printf("BuildPatchFromInfoFile error: %s\n", sError.c_str());
        return false;
    }
    else if(patchInfo.HasError())
    {
        error = patchInfo.GetError();
        error.GetErrorString(sError);
        printf("BuildPatchFromInfoFile error: %s\n", sError.c_str());
        return false;
    }

    return BuildPatch(&patchInfo, NULL, pPatchDestinationDirPath, pPatchLocalDirPath);
}


// Build a patch from a .eaPatchImpl URL.
// A .eaPatchImpl file is a patch implementation (PatchImpl struct). It is what defines a patch and when you build a 
// patch you use an PatchImpl to build it. By contrast a .eaPatchInfo (PatchInfo) is simply a high level description 
// of a patch, and not the patch itself.
// This is a case which an application might well encounter.
bool BuildPatchFromImplURL(const char8_t* pPatchImplURL, const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath)
{
    return BuildPatch(NULL, pPatchImplURL, pPatchDestinationDirPath, pPatchLocalDirPath);
}


// Cancel and roll back a patch.
// This is a case which an application might well encounter.
bool CancelPatch(const char8_t* pPatchDestinationDirPath)
{
    return BuildPatch(NULL, NULL, pPatchDestinationDirPath, NULL);
}



