/*H*************************************************************************************************/
/*!

    \File sample.h

    \Description
        This implements a basic application framework for the sample

    \Notes
        None.

    \Copyright
        Copyright (c)  Electronic Arts 2013.  ALL RIGHTS RESERVED.

    \Version 1.0 

*/
/*************************************************************************************************H*/

#ifndef _sample_core_h
#define _sample_core_h

/*** Include files ****************************************************************/

/*** Function Prototype ***********************************************************/

//These are implemented in the T2Host.cpp
void InitializeT2();
void UpdateT2(float timeTotal, float timeDelta);
void UninitializeT2();

//--------------------------------------------------------------------------------------
// Name: Sample
// Desc: Global class for this sample, derived from IFrameworkView and
//       implementing Initialize(), SetWindow, Load, Run , Uninitialize.
//       This reemoves the depenedency from the MS Sample Framework
//--------------------------------------------------------------------------------------

ref class Sample sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
    internal:
        Sample() {};

    public:
        virtual ~Sample() {};
    
    //IFrameworkView
    public:
        virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
        virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
        virtual void Load(Platform::String^ entryPoint);
        virtual void Run();
        virtual void Uninitialize();
};


//--------------------------------------------------------------------------------------
// Name: SampleFactory
// Desc: A factory for creating a sample. Derived from IFrameworkViewSource.
//       Implements CreateView();
//--------------------------------------------------------------------------------------

ref class SampleFactory sealed : Windows::ApplicationModel::Core::IFrameworkViewSource 
{
    internal:
        SampleFactory() {};

    public:
        virtual ~SampleFactory() {};
        virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

#endif