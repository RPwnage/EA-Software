/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/Test.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_EAPATCHCLIENTTEST_TEST_H
#define EAPATCHCLIENT_EAPATCHCLIENTTEST_TEST_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>
#include <EAIO/PathString.h>
#include <EASTL/list.h>
#include <UTFInternet/HTTPServer.h>


/////////////////////////////////////////////////////////////////////////////
// Types
/////////////////////////////////////////////////////////////////////////////

typedef eastl::list<EA::IO::Path::PathStringW> PathList; 


/////////////////////////////////////////////////////////////////////////////
// Test global variables
/////////////////////////////////////////////////////////////////////////////

extern EA::IO::Path::PathStringW gDataDirectory;                // Path to our test data directory, which is at the EAPatchClient/test/data directory.
extern EA::IO::Path::PathStringW gTempDirectory;                // System temp directory which we can write to during our tests.

extern bool gbTraceDirectoryRetrieverEvents;                    // If true, then debug trace DirectoryRetriever events.
extern bool gbTraceDirectoryRetrieverTelemetry;                 // If true, then debug trace DirectoryRetriever telemetry events.
extern bool gbTracePatchBuilderEvents;                          // If true, then debug trace PatchBuilder events.
extern bool gbTracePatchBuilderTelemetry;                       // If true, then debug trace PatchBuilder telemetry events.

extern EA::Internet::HTTPServer::Throttle gHTTPServerThrottle;  // Enables forcefully slowing down the effective Internet connection speed of our internal HTTP server. 

extern EA::Patch::String gExternalHTTPServerAddress;            // Optional external HTTP server address to use for the tests, as opposed to our internal HTTP server.
extern uint16_t          gExternalHTTPServerPort;               // Optional external HTTP server port to use for the tests, as opposed to our internal HTTP server.


/////////////////////////////////////////////////////////////////////////////
// Test functions
/////////////////////////////////////////////////////////////////////////////

int TestBasic();
int TestFile();
int TestHash();
int TestXML();
int TestHTTP();
int TestTelemetry();
int TestStorage();
int TestPatchDirectory();
int TestPatchBuilder();
int TestPatchBuilderCancel();


#endif // Header include guard
