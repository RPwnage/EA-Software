
#include "NisTelemetry.h"

#include "EATDF/time.h"
#include "BlazeSDK/util/telemetryapi.h"

#include "DirtySDK/dirtysock/netconn.h"

//#define EASTDC_TIMEVAL_NEEDED
#include <EAStdC/EADateTime.h>

#include <time.h>

namespace NonInteractiveSamples
{
//Send telemetry hooks to the server in the main loop
#define SPAM_TELEMETRY_HOOKS    1

 //! Macro to provide and easy way to display the token in character format
#define TELEMETRY_LocalizerTokenPrintCharArray(uToken)  \
    (char)(((uToken)>>24)&0xFF), (char)(((uToken)>>16)&0xFF), (char)(((uToken)>>8)&0xFF), (char)((uToken)&0xFF)

NisTelemetry::NisTelemetry(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisTelemetry::~NisTelemetry()
{
    if (mTelemetryApiRef != nullptr)
    {
        TelemetryApiDestroy(mTelemetryApiRef);
        mTelemetryApiRef = nullptr;
    }
}

void NisTelemetry::onCreateApis()
{
    getUiDriver().addDiagnosticFunction();

    // Create the locker component so we can use it in this app
    Blaze::Util::UtilComponent::createComponent(getBlazeHub());

    mTelemetryApiRef = nullptr;

    // To get an authenticated telemetry handle, set to true
    mAutoLogin = false;
}

void NisTelemetry::onCleanup()
{
}

void NisTelemetry::onRun()
{
    getUiDriver().addDiagnosticFunction();

    PreLogin();
    Update();
}

void NisTelemetry::PreLogin(void)
{
    if(!mAutoLogin) 
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "REQUESTING ANONYMOUS TELEMETRY REF\n");

        // Create a TelemetryApiRefT
        mTelemetryApiRef = TelemetryApiCreateEx(64, TELEMETRY_BUFFERTYPE_CIRCULAROVERWRITE, 4096);

        // Set list of disabled countries
        strncpy(mDisabledCountries,"AD,AF,AG,AI,AL,AM,AN,AO,AQ,AR,AS,AW,AX,AZ,BA,BB,BD,BF,BH,BI,BJ,BM,BN,BO,BR,BS,BT,BV,BW,BY,BZ,CC,CD,CF,CG,CI,CK,CL,CM,CN,CO,CR,CU,CV,CX,DJ,DM,DO,DZ,EC,EG,EH,ER,ET,FJ,FK,FM,FO,GA,GD,GE,GF,GG,GH,GI,GL,GM,GN,GP,GQ,GS,GT,GU,GW,GY,HM,HN,HT,ID,IL,IM,IN,IO,IQ,IR,IS,JE,JM,JO,KE,KG,KH,KI,KM,KN,KP,KR,KW,KY,KZ,LA,LB,LC,LI,LK,LR,LS,LY,MA,MC,MD,ME,MG,MH,ML,MM,MN,MO,MP,MQ,MR,MS,MU,MV,MW,MY,MZ,NA,NC,NE,NF,NG,NI,NP,NR,NU,OM,PA,PE,PF,PG,PH,PK,PM,PN,PS,PW,PY,QA,RE,RS,RW,SA,SB,SC,SD,SG,SH,SJ,SL,SM,SN,SO,SR,ST,SV,SY,SZ,TC,TD,TF,TG,TH,TJ,TK,TL,TM,TN,TO,TT,TV,TZ,UA,UG,UM,UY,UZ,VA,VC,VE,VG,VN,VU,WF,WS,YE,YT,ZM,ZW,ZZ",sizeof(mDisabledCountries));
        TelemetryApiControl(mTelemetryApiRef, 'cdbl', 0, mDisabledCountries);

        char mybuf[640];
        TelemetryApiStatus(mTelemetryApiRef, 'cdbl', mybuf, sizeof(mybuf));

        Blaze::TimeValue pTV;
        pTV = pTV.getTimeOfDay();

        // Call Anonymous authentication method
        //TelemetryApiAuthentAnonymous(mTelemetryApiRef,"159.153.239.205",9988,'enUS',"PC/dutch", (uint32_t)pTV.getSec(),"geoffster64");

        TelemetryApiAuthentAnonymousEx(mTelemetryApiRef,"10.11.20.163",9980,0,"XBL2/a", (uint32_t)pTV.getSec(),"b", nullptr, 0);

        TelemetryApiEvent(mTelemetryApiRef,'TEST','BEGN',"started");

        TelemetryApiControl(mTelemetryApiRef,'logc','x',0);

        // Connect
        TelemetryApiConnect(mTelemetryApiRef);

        // Un-halt telemetry
        TelemetryApiControl(mTelemetryApiRef, 'halt', 0, nullptr);
        TelemetryApiControl(mTelemetryApiRef, 'thrs', 0, nullptr);
        TelemetryApiControl(mTelemetryApiRef, 'cryp', 0, nullptr);
        TelemetryApiControl(mTelemetryApiRef, 'stim', 0, nullptr);
        TelemetryApiControl(mTelemetryApiRef, 'flsh', 0, nullptr);
    }
    else
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "REQUESTING STANDARD TELEMETRY REF\n");

        // Create the telemetry API throubh BlazeSDK
        Blaze::Telemetry::TelemetryAPI::TelemetryApiParams params(64,Blaze::Telemetry::TelemetryAPI::BUFFERTYPE_FILLANDSTOP,1);
        Blaze::Telemetry::TelemetryAPI::createAPI((*getBlazeHub()),params);

        // Get the handle
        mTelemetryApiRef = getBlazeHub()->getTelemetryAPI(0)->getTelemetryApiRefT();

#if DIRTYCODE_DEBUG
        TelemetryApiFiltersPrint(mTelemetryApiRef);
#endif
    }
}

//------------------------------------------------------------------------------
// Wrapper for submitting a telemetry 3.0 event
int32_t NisTelemetry::SubmitEvent(TelemetryApiRefT *pTelemetryRef)
{
    TelemetryApiEvent3T event3;
    int32_t result;

    uint32_t test3 = 8234;

    char custumStr[5] = {0};
    sprintf(custumStr, "%04X", test3); // 202A

    uint32_t final = ((uint32_t)custumStr[0] << 24) + 
        ((uint32_t)custumStr[1] << 16) + 
        ((uint32_t)custumStr[2] << 8) + 
        ((uint32_t)custumStr[3]);

    result = TelemetryApiInitEvent3(&event3,'TEST','TLM3',final);
    if(result != TC_OKAY) {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "ERR: TeslemetryApiInitEvent3 returned %d \n", result);
    }
    result = TelemetryApiEncAttributeInt(&event3, 'data', 0);
    if(result != TC_OKAY) {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "ERR: TelemetryApiEncAttributeString returned %d \n",result);
    }

 /*   result = TelemetryApiSubmitEvent3Ex(pTelemetryRef,&event3, 
        2,  
        'lstr', TELEMETRY3_ATTRIBUTE_TYPE_STRING, "This is a very long string. Longer than 256 characters in fact. Therefore is cannot be used in a TelemetryApiEncAttributeString() call. It can however, be added to an event like this. So if you need to add a long string to an event, you can use TelemetryApiSubmitEvent3Ex() or TelemetryApiSubmitEvent3Long().",  
        'flt1', TELEMETRY3_ATTRIBUTE_TYPE_FLOAT, 1.2, 
        'flt2', TELEMETRY3_ATTRIBUTE_TYPE_CHAR, 'c', 
        'flt3', TELEMETRY3_ATTRIBUTE_TYPE_INT, 123);
        */

    result = TelemetryApiSubmitEvent3(pTelemetryRef, &event3);

    if(result <= 0) 
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "ERR: TelemetryApiSubmitEvent3 returned %d \n",result);
    }
    else
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "TelemetryApiSubmitEvent3 bytes written: %d \n",result);
        return 1;
    }

    return 0;
}

//------------------------------------------------------------------------------
// Base class calls this once per frame
void NisTelemetry::Update(void)
{
    static int counter = 0;
 //   int32_t result;

    char param[15];

    static time_t sendTime = 0;
    time_t rawtime;
    TelemetryApiEvent3T event3;

    if(mTelemetryApiRef)
    {
        time ( &rawtime );

        if (sendTime == 0)
        {
            TelemetryApiInitEvent3(&event3,'STRT','TLM3','STRT');
            TelemetryApiSubmitEvent3(mTelemetryApiRef, &event3);
        }

        if( rawtime - sendTime >= 4)
        {
            sendTime = rawtime;
            SubmitEvent(mTelemetryApiRef);
            sprintf(param,"STEP.%d",counter);
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Submitting event %s\n",param);
        }

        TelemetryApiUpdate(mTelemetryApiRef);
    }
    
    counter++;
}

}
