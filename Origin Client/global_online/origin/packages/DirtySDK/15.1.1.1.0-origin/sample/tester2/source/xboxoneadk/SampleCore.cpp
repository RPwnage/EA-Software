/*H********************************************************************************/
/*!
    \File Sample.cpp

    \Description
        Provides functionality to start the sample

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/01/2012 (akirchner) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include "DirtySDK/platform.h"

#include <string>
#include <sstream>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/util/tagfield.h"

#include "SampleCore.h"

using namespace Platform;

using namespace Windows::Data;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;

using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Activation;

/*F********************************************************************************/
/*!
    \Function Sample::Initialize

    \Description
        Implements Initialize from IFrameworkView. Activates application view.
        Subscribes to Suspending and Resuming callback

    \Input  applicationView   Windows::ApplicationModel::Core::CoreApplicationView
    
    \Output None

    \Version 16/05/2013 (tcho)
/*/
/********************************************************************************F*/
void Sample::Initialize( Windows::ApplicationModel::Core::CoreApplicationView^ applicationView )
{
    applicationView->Activated += ref new Windows::Foundation::TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>([](CoreApplicationView^, IActivatedEventArgs^ args) {
            Windows::UI::Core::CoreWindow::GetForCurrentThread()->Activate();
        });
}

/*F********************************************************************************/
/*!
    \Function Sample::SetWindow

    \Description
        Implements SetWindow from IFrameworkView. (Doesn't do anything)

    \Input  window   Windows::UI::Core::CoreWindow
    
    \Output None

    \Version 16/05/2013 (tcho)
/*/
/********************************************************************************F*/
void Sample::SetWindow( Windows::UI::Core::CoreWindow^ /*window*/ )
{

}

/*F********************************************************************************/
/*!
    \Function Sample::Load

    \Description
        Implements Load from IFrameworkView. (Doesn't do anything)

    \Input  str  Platform::String
    
    \Output None

    \Version 16/05/2013 (tcho)
/*/
/********************************************************************************F*/
void Sample::Load( Platform::String^ /*str*/ )
{

}

/*F********************************************************************************/
/*!
    \Function Sample::Run

    \Description
        Implements Run from IFrameworkView.

    \Input  None
    
    \Output None

    \Version 16/05/2013 (tcho)
/*/
/********************************************************************************F*/
void Sample::Run()
{
    //Wait 3 seconds. This seems to reduce the probability of getting a fatal exception
    Sleep(3000);

    //Initialize T2
    InitializeT2();

    for(;;)
    {
        Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);

        //Give time for other threads
        NetConnSleep(16);

        //Update (must be call periodically if not T2 Host does not respond)
        UpdateT2(0,0);
    }
}

/*F********************************************************************************/
/*!
    \Function Sample::Uninitialize

    \Description
        Implements Uninitialize from IFrameworkView. Shutdown NetConn

    \Input  None
    
    \Output None

    \Version 16/05/2013 (tcho)
*/
/********************************************************************************F*/
void Sample::Uninitialize()
{
    UninitializeT2();
}

/*F********************************************************************************/
/*!
    \Function SampleFactory::CreateView

    \Description
        Create the sample View

    \Input  None
    
    \Output Returns the sample view

    \Version 16/05/2013 (tcho)
*/
/********************************************************************************F*/
Windows::ApplicationModel::Core::IFrameworkView^ SampleFactory::CreateView()
{
    return ref new Sample();
}
