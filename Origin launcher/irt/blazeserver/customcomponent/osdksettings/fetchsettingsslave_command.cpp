/*************************************************************************************************/
/*!
    \file   fetchsettingsslave_command.cpp

    $Header: //gosdev/games/Ping/V8_Gen4/ML/blazeserver/15.x/customcomponent/osdksettings/fetchsettingsslave_command.cpp#5 $
    $Change: 1204422 $
    $DateTime: 2016/06/22 00:32:41 $

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchSettingsSlaveCommand

    Requests settings information from the slave.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdksettings/rpc/osdksettingsslave/fetchsettings_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/userinfo.h"
#include "framework/util/locales.h"
#include "framework/util/localization.h"

// osdksettings includes
#include "osdksettingsslaveimpl.h"
#include "osdksettings/tdf/osdksettingstypes.h"
#include "osdksettings/osdksettingsliterals.h"

namespace Blaze
{
namespace OSDKSettings
{

    class FetchSettingsSlaveCommand : public FetchSettingsCommandStub
    {
        /*** Public Methods ******************************************************************************/
    public:
        FetchSettingsSlaveCommand(Message* message, FetchSettingsRequest* request, OSDKSettingsSlaveImpl* componentImpl)
            : FetchSettingsCommandStub(message, request), mComponent(componentImpl)
        { }

        virtual ~FetchSettingsSlaveCommand() { }

        /*** Private methods *****************************************************************************/
    private:
        /*************************************************************************************************/
        /*!
            \brief checkLiteralEmail

            Checks for the existence of an email literal in default field (see osdksettingsliterals.h).
        */
        /*************************************************************************************************/
        bool checkLiteralEmail(const char8_t* defaultStr)
        {
            // EMAIL ADDRESS - "___NUCLEUS_EMAIL" in Default field
            if (blaze_strcmp(defaultStr, OSDKSETTINGS_LITERALS[ENUM_EMAIL_ADDRESS]) == 0)
            {
                //ASSERT(gCurrentUserSession->getUserInfo() != NULL);
                //if (gCurrentUserSession->getUserInfo() != NULL)
                {
                    return true;
                }
            }
            return false;
        }
        
        /*************************************************************************************************/
        /*!
            \brief localizeStr

            Localizes a string if necessary.
        */
        /*************************************************************************************************/
        const char8_t* localizeStr(const char8_t* configStr)
        {
            uint32_t locale = gCurrentUserSession->getSessionLocale();
            return gLocalization->localize(configStr, locale);
        }

        FetchSettingsCommandStub::Errors execute()
        {
            TRACE_LOG("[FetchSettingsSlaveCommand:" << this << "].execute()");

            const OSDKSettingsConfigTdf& config = mComponent->getSettingsConfig();

            SettingValueList& responseValuePtr = mResponse.getSettingValueList();
            for (SettingValueList::const_iterator it = config.getSettings().begin();
                it != config.getSettings().end(); ++it)
            {
                SettingValue* thisSetting = responseValuePtr.pull_back();
                (*it)->copyInto(*thisSetting);

                if (false == thisSetting->getLocalized())
                {
					// localize label
					const char8_t* settingLabelLoc = localizeStr( thisSetting->getLabel() );
					if ( settingLabelLoc != thisSetting->getLabel() )
					{
						thisSetting->setLabel(settingLabelLoc);
						thisSetting->setHasLocalizedLabel(true);
					}

					// localize help label
					const char8_t* settingHelpLabelLoc = localizeStr( thisSetting->getHelpLabel() );
					if ( settingHelpLabelLoc != thisSetting->getHelpLabel() )
					{
						thisSetting->setHelpLabel(settingHelpLabelLoc);
						thisSetting->setHasLocalizedHelpLabel(true);
					}

					// localize value label(s)
					bool allValueLabelsLocalized = true;
					for (SettingValue::StringPossibleValueLabelsList::iterator itInner = thisSetting->getPossibleValueLabels().begin();
					itInner != thisSetting->getPossibleValueLabels().end(); ++itInner)
					{
						const char8_t* possibleValueStr = *itInner;
						const char8_t* possibleValueLoc = localizeStr(possibleValueStr);
						if (possibleValueLoc != possibleValueStr)
						{
							*itInner = possibleValueLoc;
						}
						else
						{
							allValueLabelsLocalized = false;
						}
					}
                
					if (allValueLabelsLocalized)
					{
						thisSetting->setHasLocalizedValueLabel(true);
					}
                }
            }

            return ERR_OK;
        }

    private:
        // Not owned memory.
        OSDKSettingsSlaveImpl* mComponent;
    };


    // static factory method impl
    FetchSettingsCommandStub* FetchSettingsCommandStub::create(Message *msg, FetchSettingsRequest* request, OSDKSettingsSlave *component)
    {
        return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "FetchSettingsSlaveCommand") FetchSettingsSlaveCommand(msg, request, static_cast<OSDKSettingsSlaveImpl*>(component));
    }

} // OSDKSettings
} // Blaze
