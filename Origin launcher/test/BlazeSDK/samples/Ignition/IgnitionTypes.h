
#ifndef IGNITIONTYPES_H
#define IGNITIONTYPES_H

#include "PyroSDK/pyrosdk.h"

namespace Ignition
{
    enum MatchmakingSessionType
    {
        FIND,
        CREATE,
        FIND_OR_CREATE
    };

    enum PlayerCapacityChangeTeamSetup
    {
        ASSIGN_LOCAL_CONNECTION_GROUP_TO_SAME_TEAM,
        SEPERATE_LOCAL_CONNECTION_GROUP,
        NO_TEAM_ASSIGNMENTS
    };

    enum CreateGameTeamAndRoleSetup
    {
        DEFAULT_ROLES,
        SIMPLE_ROLES,
        FIFA_ROLES
    };
}

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/version.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "EATDF/tdfmemberinfo.h"
#include "BlazeSDK/component/framework/tdf/userdefines.h"
#include "BlazeSDK/component/framework/tdf/qosdatatypes.h"
#include "BlazeSDK/component/util/tdf/utiltypes.h"
#include "BlazeSDK/component/messaging/tdf/messagingtypes.h"
#include "BlazeSDK/apidefinitions.h"
#include "BlazeSDK/component/authentication/tdf/accountdefines.h"
#include "BlazeSDK/component/authentication/tdf/nucleuscodes.h"
#include "BlazeSDK/component/authentication/tdf/authentication.h"
#include "BlazeSDK/component/redirector/tdf/redirectortypes.h"
#include "BlazeSDK/component/framework/tdf/network.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
#include "BlazeSDK/component/gamemanager/tdf/matchmaker_types.h"
#include "BlazeSDK/component/gamemanager/tdf/legacyrules.h"
#include "BlazeSDK/component/stats/tdf/stats.h"
#include "BlazeSDK/component/clubs/tdf/clubs.h"
#include "BlazeSDK/component/clubs/tdf/clubs_base.h"
#include "BlazeSDK/component/gamereporting/tdf/gamereporting.h"
#include "BlazeSDK/util/telemetryapi.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h" 
#include "BlazeSDK/component/associationlists/tdf/associationlists.h"
#include "BlazeSDK/component/bytevault/tdf/bytevault_base.h"
#include "BlazeSDK/component/achievements/tdf/achievements.h"
#include "BlazeSDK/localize.h"

namespace Pyro
{
    template<>
    class PyroCustomEnumValues<Blaze::EnvironmentType> : public EnumNameValueCollectionT<Blaze::EnvironmentType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::EnvironmentType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::EnvironmentType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues() : EnumNameValueCollectionT<Blaze::EnvironmentType>("Blaze::EnvironmentType")
            {
                add("ENVIRONMENT_SDEV", (int32_t)Blaze::ENVIRONMENT_SDEV);
                add("ENVIRONMENT_STEST", (int32_t)Blaze::ENVIRONMENT_STEST);
                add("ENVIRONMENT_SCERT", (int32_t)Blaze::ENVIRONMENT_SCERT);
                add("ENVIRONMENT_PROD", (int32_t)Blaze::ENVIRONMENT_PROD);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::ClientType> : public EnumNameValueCollectionT<Blaze::ClientType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::ClientType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::ClientType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::ClientType>("Blaze::ClientType")
            {
                add("CLIENT_TYPE_GAMEPLAY_USER", (int32_t)Blaze::CLIENT_TYPE_GAMEPLAY_USER);
                add("CLIENT_TYPE_HTTP_USER", (int32_t)Blaze::CLIENT_TYPE_HTTP_USER);
                add("CLIENT_TYPE_DEDICATED_SERVER", (int32_t)Blaze::CLIENT_TYPE_DEDICATED_SERVER);
                add("CLIENT_TYPE_TOOLS", (int32_t)Blaze::CLIENT_TYPE_TOOLS);
                add("CLIENT_TYPE_LIMITED_GAMEPLAY_USER", (int32_t)Blaze::CLIENT_TYPE_LIMITED_GAMEPLAY_USER);
                add("CLIENT_TYPE_INVALID", (int32_t)Blaze::CLIENT_TYPE_INVALID);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::ClientPlatformType> : public EnumNameValueCollectionT<Blaze::ClientPlatformType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::ClientPlatformType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::ClientPlatformType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::ClientPlatformType>("Blaze::ClientPlatformType")
            {
                add("NATIVE", (int32_t)Blaze::NATIVE);
                add("INVALID", (int32_t)Blaze::INVALID);
                add("pc", (int32_t)Blaze::pc);
                add("android", (int32_t)Blaze::android);
                add("ios", (int32_t)Blaze::ios);
                add("common", (int32_t)Blaze::common);
                add("mobile", (int32_t)Blaze::mobile);
                add("legacyprofileid", (int32_t)Blaze::legacyprofileid);
                add("verizon", (int32_t)Blaze::verizon);
                add("facebook", (int32_t)Blaze::facebook);
                add("facebook_eacom", (int32_t)Blaze::facebook_eacom);
                add("bebo", (int32_t)Blaze::bebo);
                add("friendster", (int32_t)Blaze::friendster);
                add("twitter", (int32_t)Blaze::twitter);
                add("xone", (int32_t)Blaze::xone);
                add("ps4", (int32_t)Blaze::ps4);
                add("nx", (int32_t)Blaze::nx);
                add("ps5", (int32_t)Blaze::ps5);
                add("xbsx", (int32_t)Blaze::xbsx);
                add("steam", (int32_t)Blaze::steam);
                add("stadia", (int32_t)Blaze::stadia);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::UpnpStatus> : public EnumNameValueCollectionT<Blaze::UpnpStatus>
    {
        public:
            static const PyroCustomEnumValues<Blaze::UpnpStatus> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::UpnpStatus> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::UpnpStatus>("Blaze::UpnpStatus")
            {
                add("UPNP_UNKNOWN", (int32_t)Blaze::UPNP_UNKNOWN);
                add("UPNP_FOUND", (int32_t)Blaze::UPNP_FOUND);
                add("UPNP_ENABLED", (int32_t)Blaze::UPNP_ENABLED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Util::NatType> : public EnumNameValueCollectionT<Blaze::Util::NatType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Util::NatType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Util::NatType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Util::NatType>("Blaze::Util::NatType")
            {
                add("NAT_TYPE_OPEN", (int32_t)Blaze::Util::NAT_TYPE_OPEN);
                add("NAT_TYPE_MODERATE", (int32_t)Blaze::Util::NAT_TYPE_MODERATE);
                add("NAT_TYPE_STRICT_SEQUENTIAL", (int32_t)Blaze::Util::NAT_TYPE_STRICT_SEQUENTIAL);
                add("NAT_TYPE_STRICT", (int32_t)Blaze::Util::NAT_TYPE_STRICT);
                add("NAT_TYPE_UNKNOWN", (int32_t)Blaze::Util::NAT_TYPE_UNKNOWN);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Util::TelemetryOpt> : public EnumNameValueCollectionT<Blaze::Util::TelemetryOpt>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Util::TelemetryOpt> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Util::TelemetryOpt> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Util::TelemetryOpt>("Blaze::Util::TelemetryOpt")
            {
                add("TELEMETRY_OPT_OUT", (int32_t)Blaze::Util::TELEMETRY_OPT_OUT);
                add("TELEMETRY_OPT_IN", (int32_t)Blaze::Util::TELEMETRY_OPT_IN);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Util::FilterResult> : public EnumNameValueCollectionT<Blaze::Util::FilterResult>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Util::FilterResult> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Util::FilterResult> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Util::FilterResult>("Blaze::Util::FilterResult")
            {
                add("FILTER_RESULT_PASSED", (int32_t)Blaze::Util::FILTER_RESULT_PASSED);
                add("FILTER_RESULT_OFFENSIVE", (int32_t)Blaze::Util::FILTER_RESULT_OFFENSIVE);
                add("FILTER_RESULT_UNPROCESSED", (int32_t)Blaze::Util::FILTER_RESULT_UNPROCESSED);
                add("FILTER_RESULT_STRING_TOO_LONG", (int32_t)Blaze::Util::FILTER_RESULT_STRING_TOO_LONG);
                add("FILTER_RESULT_OTHER", (int32_t)Blaze::Util::FILTER_RESULT_OTHER);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Messaging::MessageOrder> : public EnumNameValueCollectionT<Blaze::Messaging::MessageOrder>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Messaging::MessageOrder> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Messaging::MessageOrder> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Messaging::MessageOrder>("Blaze::Messaging::MessageOrder")
            {
                add("ORDER_DEFAULT", (int32_t)Blaze::Messaging::ORDER_DEFAULT);
                add("ORDER_TIME_ASC", (int32_t)Blaze::Messaging::ORDER_TIME_ASC);
                add("ORDER_TIME_DESC", (int32_t)Blaze::Messaging::ORDER_TIME_DESC);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::APIIdDefinitions> : public EnumNameValueCollectionT<Blaze::APIIdDefinitions>
    {
        public:
            static const PyroCustomEnumValues<Blaze::APIIdDefinitions> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::APIIdDefinitions> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::APIIdDefinitions>("Blaze::APIIdDefinitions")
            {
                add("NULL_API", (int32_t)Blaze::NULL_API);
                add("GAMEMANAGER_API", (int32_t)Blaze::GAMEMANAGER_API);
                add("STATS_API", (int32_t)Blaze::STATS_API);
                add("LEADERBOARD_API", (int32_t)Blaze::LEADERBOARD_API);
                add("MESSAGING_API", (int32_t)Blaze::MESSAGING_API);
                add("CENSUSDATA_API", (int32_t)Blaze::CENSUSDATA_API);
                add("UTIL_API", (int32_t)Blaze::UTIL_API);
                add("TELEMETRY_API", (int32_t)Blaze::TELEMETRY_API);
                add("ASSOCIATIONLIST_API", (int32_t)Blaze::ASSOCIATIONLIST_API);
                add("FIRST_CUSTOM_API", (int32_t)Blaze::FIRST_CUSTOM_API);
                add("MAX_API_ID", (int32_t)Blaze::MAX_API_ID);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::NucleusCause::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::NucleusCause::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::NucleusCause::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::NucleusCause::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::NucleusCause::Code>("Blaze::Nucleus::NucleusCause::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::NucleusCause::UNKNOWN);
                add("INVALID_VALUE", (int32_t)Blaze::Nucleus::NucleusCause::INVALID_VALUE);
                add("ILLEGAL_VALUE", (int32_t)Blaze::Nucleus::NucleusCause::ILLEGAL_VALUE);
                add("MISSING_VALUE", (int32_t)Blaze::Nucleus::NucleusCause::MISSING_VALUE);
                add("DUPLICATE_VALUE", (int32_t)Blaze::Nucleus::NucleusCause::DUPLICATE_VALUE);
                add("INVALID_EMAIL_DOMAIN", (int32_t)Blaze::Nucleus::NucleusCause::INVALID_EMAIL_DOMAIN);
                add("SPACES_NOT_ALLOWED", (int32_t)Blaze::Nucleus::NucleusCause::SPACES_NOT_ALLOWED);
                add("TOO_SHORT", (int32_t)Blaze::Nucleus::NucleusCause::TOO_SHORT);
                add("TOO_LONG", (int32_t)Blaze::Nucleus::NucleusCause::TOO_LONG);
                add("TOO_YOUNG", (int32_t)Blaze::Nucleus::NucleusCause::TOO_YOUNG);
                add("TOO_OLD", (int32_t)Blaze::Nucleus::NucleusCause::TOO_OLD);
                add("ILLEGAL_FOR_COUNTRY", (int32_t)Blaze::Nucleus::NucleusCause::ILLEGAL_FOR_COUNTRY);
                add("BANNED_COUNTRY", (int32_t)Blaze::Nucleus::NucleusCause::BANNED_COUNTRY);
                add("NOT_ALLOWED", (int32_t)Blaze::Nucleus::NucleusCause::NOT_ALLOWED);
                add("TOKEN_EXPIRED", (int32_t)Blaze::Nucleus::NucleusCause::TOKEN_EXPIRED);
                add("OPTIN_MISMATCH", (int32_t)Blaze::Nucleus::NucleusCause::OPTIN_MISMATCH);
                add("TOO_MANY", (int32_t)Blaze::Nucleus::NucleusCause::TOO_MANY);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::NucleusField::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::NucleusField::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::NucleusField::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::NucleusField::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::NucleusField::Code>("Blaze::Nucleus::NucleusField::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::NucleusField::UNKNOWN);
                add("PASSWORD", (int32_t)Blaze::Nucleus::NucleusField::PASSWORD);
                add("EMAIL", (int32_t)Blaze::Nucleus::NucleusField::EMAIL);
                add("PARENTAL_EMAIL", (int32_t)Blaze::Nucleus::NucleusField::PARENTAL_EMAIL);
                add("DISPLAY_NAME", (int32_t)Blaze::Nucleus::NucleusField::DISPLAY_NAME);
                add("STATUS", (int32_t)Blaze::Nucleus::NucleusField::STATUS);
                add("DOB", (int32_t)Blaze::Nucleus::NucleusField::DOB);
                add("TOKEN", (int32_t)Blaze::Nucleus::NucleusField::TOKEN);
                add("EXPIRATION", (int32_t)Blaze::Nucleus::NucleusField::EXPIRATION);
                add("OPTINTYPE", (int32_t)Blaze::Nucleus::NucleusField::OPTINTYPE);
                add("APPLICATION", (int32_t)Blaze::Nucleus::NucleusField::APPLICATION);
                add("SOURCE", (int32_t)Blaze::Nucleus::NucleusField::SOURCE);
                add("BASENAME", (int32_t)Blaze::Nucleus::NucleusField::BASENAME);
                add("NUMBERSUGGESTIONS", (int32_t)Blaze::Nucleus::NucleusField::NUMBERSUGGESTIONS);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::NucleusCode::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::NucleusCode::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::NucleusCode::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::NucleusCode::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::NucleusCode::Code>("Blaze::Nucleus::NucleusCode::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::NucleusCode::UNKNOWN);
                add("VALIDATION_FAILED", (int32_t)Blaze::Nucleus::NucleusCode::VALIDATION_FAILED);
                add("NO_SUCH_USER", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_USER);
                add("NO_SUCH_COUNTRY", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_COUNTRY);
                add("NO_SUCH_PERSONA", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_PERSONA);
                add("NO_SUCH_EXTERNAL_REFERENCE", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_EXTERNAL_REFERENCE);
                add("NO_SUCH_NAMESPACE", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_NAMESPACE);
                add("INVALID_PASSWORD", (int32_t)Blaze::Nucleus::NucleusCode::INVALID_PASSWORD);
                add("CODE_ALREADY_USED", (int32_t)Blaze::Nucleus::NucleusCode::CODE_ALREADY_USED);
                add("INVALID_CODE", (int32_t)Blaze::Nucleus::NucleusCode::INVALID_CODE);
                add("PARSE_EXCEPTION", (int32_t)Blaze::Nucleus::NucleusCode::PARSE_EXCEPTION);
                add("NO_SUCH_GROUP", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_GROUP);
                add("NO_SUCH_GROUP_NAME", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_GROUP_NAME);
                add("NO_ASSOCIATED_PRODUCT", (int32_t)Blaze::Nucleus::NucleusCode::NO_ASSOCIATED_PRODUCT);
                add("CODE_ALREADY_DISABLED", (int32_t)Blaze::Nucleus::NucleusCode::CODE_ALREADY_DISABLED);
                add("GROUP_NAME_DOES_NOT_MATCH", (int32_t)Blaze::Nucleus::NucleusCode::GROUP_NAME_DOES_NOT_MATCH);
                add("NO_SUCH_OPTIN", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_OPTIN);
                add("INVALID_OPTIN", (int32_t)Blaze::Nucleus::NucleusCode::INVALID_OPTIN);
                add("NO_SUCH_PERSONA_REFERENCE", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_PERSONA_REFERENCE);
                add("NO_SUCH_AUTH_DATA", (int32_t)Blaze::Nucleus::NucleusCode::NO_SUCH_AUTH_DATA);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::ProductCatalog::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::ProductCatalog::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::ProductCatalog::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::ProductCatalog::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::ProductCatalog::Code>("Blaze::Nucleus::ProductCatalog::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::ProductCatalog::UNKNOWN);
                add("SKUD", (int32_t)Blaze::Nucleus::ProductCatalog::SKUD);
                add("OFB", (int32_t)Blaze::Nucleus::ProductCatalog::OFB);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::StatusReason::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::StatusReason::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::StatusReason::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::StatusReason::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::StatusReason::Code>("Blaze::Nucleus::StatusReason::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::StatusReason::UNKNOWN);
                add("NONE", (int32_t)Blaze::Nucleus::StatusReason::NONE);
                add("REACTIVATED_CUSTOMER", (int32_t)Blaze::Nucleus::StatusReason::REACTIVATED_CUSTOMER);
                add("INVALID_EMAIL", (int32_t)Blaze::Nucleus::StatusReason::INVALID_EMAIL);
                add("PRIVACY_POLICY", (int32_t)Blaze::Nucleus::StatusReason::PRIVACY_POLICY);
                add("PARENTS_REQUEST", (int32_t)Blaze::Nucleus::StatusReason::PARENTS_REQUEST);
                add("PARENTAL_REQUEST", (int32_t)Blaze::Nucleus::StatusReason::PARENTAL_REQUEST);
                add("SUSPENDED_MISCONDUCT_GENERAL", (int32_t)Blaze::Nucleus::StatusReason::SUSPENDED_MISCONDUCT_GENERAL);
                add("SUSPENDED_MISCONDUCT_HARASSMENT", (int32_t)Blaze::Nucleus::StatusReason::SUSPENDED_MISCONDUCT_HARASSMENT);
                add("SUSPENDED_MISCONDUCT_MACROING", (int32_t)Blaze::Nucleus::StatusReason::SUSPENDED_MISCONDUCT_MACROING);
                add("SUSPENDED_MISCONDUCT_EXPLOITATION", (int32_t)Blaze::Nucleus::StatusReason::SUSPENDED_MISCONDUCT_EXPLOITATION);
                add("CUSTOMER_OPT_OUT", (int32_t)Blaze::Nucleus::StatusReason::CUSTOMER_OPT_OUT);
                add("CUSTOMER_UNDER_AGE", (int32_t)Blaze::Nucleus::StatusReason::CUSTOMER_UNDER_AGE);
                add("EMAIL_CONFIRMATION_REQUIRED", (int32_t)Blaze::Nucleus::StatusReason::EMAIL_CONFIRMATION_REQUIRED);
                add("MISTYPED_ID", (int32_t)Blaze::Nucleus::StatusReason::MISTYPED_ID);
                add("ABUSED_ID", (int32_t)Blaze::Nucleus::StatusReason::ABUSED_ID);
                add("DEACTIVATED_EMAIL_LINK", (int32_t)Blaze::Nucleus::StatusReason::DEACTIVATED_EMAIL_LINK);
                add("DEACTIVATED_CS", (int32_t)Blaze::Nucleus::StatusReason::DEACTIVATED_CS);
                add("CLAIMED_BY_TRUE_OWNER", (int32_t)Blaze::Nucleus::StatusReason::CLAIMED_BY_TRUE_OWNER);
                add("BANNED", (int32_t)Blaze::Nucleus::StatusReason::BANNED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::EntitlementStatus::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::EntitlementStatus::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::EntitlementStatus::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::EntitlementStatus::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::EntitlementStatus::Code>("Blaze::Nucleus::EntitlementStatus::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::EntitlementStatus::UNKNOWN);
                add("ACTIVE", (int32_t)Blaze::Nucleus::EntitlementStatus::ACTIVE);
                add("DISABLED", (int32_t)Blaze::Nucleus::EntitlementStatus::DISABLED);
                add("PENDING", (int32_t)Blaze::Nucleus::EntitlementStatus::PENDING);
                add("DELETED", (int32_t)Blaze::Nucleus::EntitlementStatus::DELETED);
                add("BANNED", (int32_t)Blaze::Nucleus::EntitlementStatus::BANNED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::PersonaStatus::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::PersonaStatus::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::PersonaStatus::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::PersonaStatus::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::PersonaStatus::Code>("Blaze::Nucleus::PersonaStatus::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::PersonaStatus::UNKNOWN);
                add("PENDING", (int32_t)Blaze::Nucleus::PersonaStatus::PENDING);
                add("ACTIVE", (int32_t)Blaze::Nucleus::PersonaStatus::ACTIVE);
                add("DEACTIVATED", (int32_t)Blaze::Nucleus::PersonaStatus::DEACTIVATED);
                add("DISABLED", (int32_t)Blaze::Nucleus::PersonaStatus::DISABLED);
                add("DELETED", (int32_t)Blaze::Nucleus::PersonaStatus::DELETED);
                add("BANNED", (int32_t)Blaze::Nucleus::PersonaStatus::BANNED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::EntitlementType::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::EntitlementType::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::EntitlementType::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::EntitlementType::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::EntitlementType::Code>("Blaze::Nucleus::EntitlementType::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::EntitlementType::UNKNOWN);
                add("ONLINE_ACCESS", (int32_t)Blaze::Nucleus::EntitlementType::ONLINE_ACCESS);
                add("TRIAL_ONLINE_ACCESS", (int32_t)Blaze::Nucleus::EntitlementType::TRIAL_ONLINE_ACCESS);
                add("SUBSCRIPTIONS", (int32_t)Blaze::Nucleus::EntitlementType::SUBSCRIPTIONS);
                add("PARENTAL_APPROVAL", (int32_t)Blaze::Nucleus::EntitlementType::PARENTAL_APPROVAL);
                add("DEFAULT", (int32_t)Blaze::Nucleus::EntitlementType::DEFAULT);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::EmailStatus::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::EmailStatus::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::EmailStatus::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::EmailStatus::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::EmailStatus::Code>("Blaze::Nucleus::EmailStatus::Code")
            {
                add("BAD", (int32_t)Blaze::Nucleus::EmailStatus::BAD);
                add("UNKNOWN", (int32_t)Blaze::Nucleus::EmailStatus::UNKNOWN);
                add("VERIFIED", (int32_t)Blaze::Nucleus::EmailStatus::VERIFIED);
                add("GUEST", (int32_t)Blaze::Nucleus::EmailStatus::GUEST);
                add("ANONYMOUS", (int32_t)Blaze::Nucleus::EmailStatus::ANONYMOUS);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::AccountStatus::Code> : public EnumNameValueCollectionT<Blaze::Nucleus::AccountStatus::Code>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::AccountStatus::Code> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::AccountStatus::Code> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::AccountStatus::Code>("Blaze::Nucleus::AccountStatus::Code")
            {
                add("UNKNOWN", (int32_t)Blaze::Nucleus::AccountStatus::UNKNOWN);
                add("ACTIVE", (int32_t)Blaze::Nucleus::AccountStatus::ACTIVE);
                add("BANNED", (int32_t)Blaze::Nucleus::AccountStatus::BANNED);
                add("CHILD_APPROVED", (int32_t)Blaze::Nucleus::AccountStatus::CHILD_APPROVED);
                add("CHILD_PENDING", (int32_t)Blaze::Nucleus::AccountStatus::CHILD_PENDING);
                add("DEACTIVATED", (int32_t)Blaze::Nucleus::AccountStatus::DEACTIVATED);
                add("DELETED", (int32_t)Blaze::Nucleus::AccountStatus::DELETED);
                add("DISABLED", (int32_t)Blaze::Nucleus::AccountStatus::DISABLED);
                add("PENDING", (int32_t)Blaze::Nucleus::AccountStatus::PENDING);
                add("TENTATIVE", (int32_t)Blaze::Nucleus::AccountStatus::TENTATIVE);                
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Nucleus::EntitlementSearchType::Type> : public EnumNameValueCollectionT<Blaze::Nucleus::EntitlementSearchType::Type>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Nucleus::EntitlementSearchType::Type> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Nucleus::EntitlementSearchType::Type> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Nucleus::EntitlementSearchType::Type>("Blaze::Nucleus::EntitlementSearchType::Type")
            {
                add("USER", (int32_t)Blaze::Nucleus::EntitlementSearchType::USER);
                add("PERSONA", (int32_t)Blaze::Nucleus::EntitlementSearchType::PERSONA);
                add("DEVICE", (int32_t)Blaze::Nucleus::EntitlementSearchType::DEVICE);
                add("USER_OR_PERSONA", (int32_t)Blaze::Nucleus::EntitlementSearchType::USER_OR_PERSONA);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Redirector::InstanceType> : public EnumNameValueCollectionT<Blaze::Redirector::InstanceType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Redirector::InstanceType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Redirector::InstanceType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Redirector::InstanceType>("Blaze::Redirector::InstanceType")
            {
                add("SLAVE", (int32_t)Blaze::Redirector::SLAVE);
                add("AUX_MASTER", (int32_t)Blaze::Redirector::AUX_MASTER);
                add("CONFIG_MASTER", (int32_t)Blaze::Redirector::CONFIG_MASTER);
                add("AUX_SLAVE", (int32_t)Blaze::Redirector::AUX_SLAVE);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Redirector::ServerAddressType> : public EnumNameValueCollectionT<Blaze::Redirector::ServerAddressType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Redirector::ServerAddressType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Redirector::ServerAddressType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Redirector::ServerAddressType>("Blaze::Redirector::ServerAddressType")
            {
                add("INTERNAL_IPPORT", (int32_t)Blaze::Redirector::INTERNAL_IPPORT);
                add("EXTERNAL_IPPORT", (int32_t)Blaze::Redirector::EXTERNAL_IPPORT);
                add("XBOX_SERVER_ADDRESS", (int32_t)Blaze::Redirector::XBOX_SERVER_ADDRESS);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::VoipTopology> : public EnumNameValueCollectionT<Blaze::VoipTopology>
    {
        public:
            static const PyroCustomEnumValues<Blaze::VoipTopology> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::VoipTopology> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::VoipTopology>("Blaze::VoipTopology")
            {
                add("VOIP_DISABLED", (int32_t)Blaze::VOIP_DISABLED);
                add("VOIP_DEDICATED_SERVER", (int32_t)Blaze::VOIP_DEDICATED_SERVER);
                add("VOIP_PEER_TO_PEER", (int32_t)Blaze::VOIP_PEER_TO_PEER);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameNetworkTopology> : public EnumNameValueCollectionT<Blaze::GameNetworkTopology>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameNetworkTopology> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameNetworkTopology> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameNetworkTopology>("Blaze::GameNetworkTopology")
            {
                add("CLIENT_SERVER_PEER_HOSTED", (int32_t)Blaze::CLIENT_SERVER_PEER_HOSTED);
                add("CLIENT_SERVER_DEDICATED", (int32_t)Blaze::CLIENT_SERVER_DEDICATED);
                add("PEER_TO_PEER_FULL_MESH", (int32_t)Blaze::PEER_TO_PEER_FULL_MESH);
                add("NETWORK_DISABLED", (int32_t)Blaze::NETWORK_DISABLED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::PresenceMode> : public EnumNameValueCollectionT<Blaze::PresenceMode>
    {
        public:
            static const PyroCustomEnumValues<Blaze::PresenceMode> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::PresenceMode> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::PresenceMode>("Blaze::PresenceMode")
            {
                add("PRESENCE_MODE_NONE", (int32_t)Blaze::PRESENCE_MODE_NONE);
                add("PRESENCE_MODE_STANDARD", (int32_t)Blaze::PRESENCE_MODE_STANDARD);
                add("PRESENCE_MODE_PRIVATE", (int32_t)Blaze::PRESENCE_MODE_PRIVATE);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::BindType> : public EnumNameValueCollectionT<Blaze::BindType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::BindType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::BindType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::BindType>("Blaze::BindType")
            {
                add("BIND_ALL", (int32_t)Blaze::BIND_ALL);
                add("BIND_INTERNAL", (int32_t)Blaze::BIND_INTERNAL);
                add("BIND_EXTERNAL", (int32_t)Blaze::BIND_EXTERNAL);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Util::ProfanityFilterType> : public EnumNameValueCollectionT<Blaze::Util::ProfanityFilterType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Util::ProfanityFilterType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Util::ProfanityFilterType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Util::ProfanityFilterType>("Blaze::Util::ProfanityFilterType")
            {
                add("FILTER_CLIENT_SERVER", (int32_t)Blaze::Util::FILTER_CLIENT_SERVER);
                add("FILTER_CLIENT_ONLY", (int32_t)Blaze::Util::FILTER_CLIENT_ONLY);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::GameState> : public EnumNameValueCollectionT<Blaze::GameManager::GameState>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::GameState> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::GameState> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::GameState>("Blaze::GameManager::GameState")
            {
                add("NEW_STATE", (int32_t)Blaze::GameManager::NEW_STATE);
                add("INITIALIZING", (int32_t)Blaze::GameManager::INITIALIZING);
                add("INACTIVE_VIRTUAL", (int32_t)Blaze::GameManager::INACTIVE_VIRTUAL);
                add("PRE_GAME", (int32_t)Blaze::GameManager::PRE_GAME);
                add("IN_GAME", (int32_t)Blaze::GameManager::IN_GAME);
                add("POST_GAME", (int32_t)Blaze::GameManager::POST_GAME);
                add("MIGRATING", (int32_t)Blaze::GameManager::MIGRATING);
                add("DESTRUCTING", (int32_t)Blaze::GameManager::DESTRUCTING);
                add("RESETABLE", (int32_t)Blaze::GameManager::RESETABLE);
                add("GAME_GROUP_INITIALIZED", (int32_t)Blaze::GameManager::GAME_GROUP_INITIALIZED);
                add("UNRESPONSIVE", (int32_t)Blaze::GameManager::UNRESPONSIVE);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::GameType> : public EnumNameValueCollectionT<Blaze::GameManager::GameType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::GameType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::GameType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::GameType>("Blaze::GameManager::GameType")
            {
                add("GAME_TYPE_GAMESESSION", (int32_t)Blaze::GameManager::GAME_TYPE_GAMESESSION);
                add("GAME_TYPE_GROUP", (int32_t)Blaze::GameManager::GAME_TYPE_GROUP);
            }
    };


    template<>
    class PyroCustomEnumValues<Blaze::GameManager::JoinMethod> : public EnumNameValueCollectionT<Blaze::GameManager::JoinMethod>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::JoinMethod> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::JoinMethod> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::JoinMethod>("Blaze::GameManager::JoinMethod")
            {
                add("JOIN_BY_BROWSING", (int32_t)Blaze::GameManager::JOIN_BY_BROWSING);
                add("JOIN_BY_MATCHMAKING", (int32_t)Blaze::GameManager::JOIN_BY_MATCHMAKING);
                add("JOIN_BY_INVITES", (int32_t)Blaze::GameManager::JOIN_BY_INVITES);
                add("JOIN_BY_PLAYER", (int32_t)Blaze::GameManager::JOIN_BY_PLAYER);
                add("SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME", (int32_t)Blaze::GameManager::SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME);
                add("SYS_JOIN_BY_RESETDEDICATEDSERVER", (int32_t)Blaze::GameManager::SYS_JOIN_BY_RESETDEDICATEDSERVER);
                add("SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER", (int32_t)Blaze::GameManager::SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER);
                add("SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME_HOST", (int32_t)Blaze::GameManager::SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME_HOST);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::SlotType> : public EnumNameValueCollectionT<Blaze::GameManager::SlotType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::SlotType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::SlotType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::SlotType>("Blaze::GameManager::SlotType")
            {
                add("SLOT_PUBLIC_PARTICIPANT", (int32_t)Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT);
                add("SLOT_PRIVATE_PARTICIPANT", (int32_t)Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT);
                add("SLOT_PUBLIC_SPECTATOR", (int32_t)Blaze::GameManager::SLOT_PUBLIC_SPECTATOR);
                add("SLOT_PRIVATE_SPECTATOR", (int32_t)Blaze::GameManager::SLOT_PRIVATE_SPECTATOR);
                add("MAX_SLOT_TYPE", (int32_t)Blaze::GameManager::MAX_SLOT_TYPE);
                add("INVALID_SLOT_TYPE", (int32_t)Blaze::GameManager::INVALID_SLOT_TYPE);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::PlayerState> : public EnumNameValueCollectionT<Blaze::GameManager::PlayerState>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::PlayerState> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::PlayerState> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::PlayerState>("Blaze::GameManager::PlayerState")
            {
                add("RESERVED", (int32_t)Blaze::GameManager::RESERVED);
                add("QUEUED", (int32_t)Blaze::GameManager::QUEUED);
                add("ACTIVE_CONNECTING", (int32_t)Blaze::GameManager::ACTIVE_CONNECTING);
                add("ACTIVE_MIGRATING", (int32_t)Blaze::GameManager::ACTIVE_MIGRATING);
                add("ACTIVE_CONNECTED", (int32_t)Blaze::GameManager::ACTIVE_CONNECTED);
                add("ACTIVE_KICK_PENDING", (int32_t)Blaze::GameManager::ACTIVE_KICK_PENDING);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::PlayerNetConnectionStatus> : public EnumNameValueCollectionT<Blaze::GameManager::PlayerNetConnectionStatus>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::PlayerNetConnectionStatus> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::PlayerNetConnectionStatus> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::PlayerNetConnectionStatus>("Blaze::GameManager::PlayerNetConnectionStatus")
            {
                add("DISCONNECTED", (int32_t)Blaze::GameManager::DISCONNECTED);
                add("ESTABLISHING_CONNECTION", (int32_t)Blaze::GameManager::ESTABLISHING_CONNECTION);
                add("CONNECTED", (int32_t)Blaze::GameManager::CONNECTED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::GameDestructionReason> : public EnumNameValueCollectionT<Blaze::GameManager::GameDestructionReason>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::GameDestructionReason> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::GameDestructionReason> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::GameDestructionReason>("Blaze::GameManager::GameDestructionReason")
            {
                add("SYS_GAME_ENDING", (int32_t)Blaze::GameManager::SYS_GAME_ENDING);
                add("SYS_CREATION_FAILED", (int32_t)Blaze::GameManager::SYS_CREATION_FAILED);
                add("HOST_LEAVING", (int32_t)Blaze::GameManager::HOST_LEAVING);
                add("HOST_INJECTION", (int32_t)Blaze::GameManager::HOST_INJECTION);
                add("HOST_EJECTION", (int32_t)Blaze::GameManager::HOST_EJECTION);
                add("LOCAL_PLAYER_LEAVING", (int32_t)Blaze::GameManager::LOCAL_PLAYER_LEAVING);
                add("TITLE_REASON_BASE_GAME_DESTRUCTION_REASON", (int32_t)Blaze::GameManager::TITLE_REASON_BASE_GAME_DESTRUCTION_REASON);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::PlayerRemovedReason> : public EnumNameValueCollectionT<Blaze::GameManager::PlayerRemovedReason>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::PlayerRemovedReason> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::PlayerRemovedReason> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::PlayerRemovedReason>("Blaze::GameManager::PlayerRemovedReason")
            {
                add("PLAYER_JOIN_TIMEOUT", (int32_t)Blaze::GameManager::PLAYER_JOIN_TIMEOUT);
                add("PLAYER_CONN_LOST", (int32_t)Blaze::GameManager::PLAYER_CONN_LOST);
                add("BLAZESERVER_CONN_LOST", (int32_t)Blaze::GameManager::BLAZESERVER_CONN_LOST);
                add("MIGRATION_FAILED", (int32_t)Blaze::GameManager::MIGRATION_FAILED);
                add("GAME_DESTROYED", (int32_t)Blaze::GameManager::GAME_DESTROYED);
                add("GAME_ENDED", (int32_t)Blaze::GameManager::GAME_ENDED);
                add("PLAYER_LEFT", (int32_t)Blaze::GameManager::PLAYER_LEFT);
                add("GROUP_LEFT", (int32_t)Blaze::GameManager::GROUP_LEFT);
                add("PLAYER_KICKED", (int32_t)Blaze::GameManager::PLAYER_KICKED);
                add("PLAYER_KICKED_WITH_BAN", (int32_t)Blaze::GameManager::PLAYER_KICKED_WITH_BAN);
                add("PLAYER_KICKED_CONN_UNRESPONSIVE", (int32_t)Blaze::GameManager::PLAYER_KICKED_CONN_UNRESPONSIVE);
                add("PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN", (int32_t)Blaze::GameManager::PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN);
                add("PLAYER_KICKED_POOR_CONNECTION", (int32_t)Blaze::GameManager::PLAYER_KICKED_POOR_CONNECTION);
                add("PLAYER_KICKED_POOR_CONNECTION_WITH_BAN", (int32_t)Blaze::GameManager::PLAYER_KICKED_POOR_CONNECTION_WITH_BAN);
                add("PLAYER_JOIN_FROM_QUEUE_FAILED", (int32_t)Blaze::GameManager::PLAYER_JOIN_FROM_QUEUE_FAILED);
                add("PLAYER_RESERVATION_TIMEOUT", (int32_t)Blaze::GameManager::PLAYER_RESERVATION_TIMEOUT);
                add("HOST_EJECTED", (int32_t)Blaze::GameManager::HOST_EJECTED);
                add("PLAYER_LEFT_MAKE_RESERVATION", (int32_t)Blaze::GameManager::PLAYER_LEFT_MAKE_RESERVATION);
                add("GROUP_LEFT_MAKE_RESERVATION", (int32_t)Blaze::GameManager::GROUP_LEFT_MAKE_RESERVATION);
                add("DISCONNECT_RESERVATION_TIMEOUT", (int32_t)Blaze::GameManager::DISCONNECT_RESERVATION_TIMEOUT);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::HostMigrationType> : public EnumNameValueCollectionT<Blaze::GameManager::HostMigrationType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::HostMigrationType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::HostMigrationType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::HostMigrationType>("Blaze::GameManager::HostMigrationType")
            {
                add("TOPOLOGY_HOST_MIGRATION", (int32_t)Blaze::GameManager::TOPOLOGY_HOST_MIGRATION);
                add("PLATFORM_HOST_MIGRATION", (int32_t)Blaze::GameManager::PLATFORM_HOST_MIGRATION);
                add("TOPOLOGY_PLATFORM_HOST_MIGRATION", (int32_t)Blaze::GameManager::TOPOLOGY_PLATFORM_HOST_MIGRATION);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::GameEntryType> : public EnumNameValueCollectionT<Blaze::GameManager::GameEntryType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::GameEntryType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::GameEntryType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::GameEntryType>("Blaze::GameManager::GameEntryType")
            {
                add("GAME_ENTRY_TYPE_DIRECT", (int32_t)Blaze::GameManager::GAME_ENTRY_TYPE_DIRECT);
                add("GAME_ENTRY_TYPE_MAKE_RESERVATION", (int32_t)Blaze::GameManager::GAME_ENTRY_TYPE_MAKE_RESERVATION);
                add("GAME_ENTRY_TYPE_CLAIM_RESERVATION", (int32_t)Blaze::GameManager::GAME_ENTRY_TYPE_CLAIM_RESERVATION);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::JoinState> : public EnumNameValueCollectionT<Blaze::GameManager::JoinState>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::JoinState> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::JoinState> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::JoinState>("Blaze::GameManager::JoinState")
            {
                add("JOINED_GAME", (int32_t)Blaze::GameManager::JOINED_GAME);
                add("IN_QUEUE", (int32_t)Blaze::GameManager::IN_QUEUE);
                add("GROUP_PARTIALLY_JOINED", (int32_t)Blaze::GameManager::GROUP_PARTIALLY_JOINED);
                add("NO_ONE_JOINED", (int32_t)Blaze::GameManager::NO_ONE_JOINED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::DatalessContext> : public EnumNameValueCollectionT<Blaze::GameManager::DatalessContext>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::DatalessContext> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::DatalessContext> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::DatalessContext>("Blaze::GameManager::DatalessContext")
            {
                add("CREATE_GAME_SETUP_CONTEXT", (int32_t)Blaze::GameManager::CREATE_GAME_SETUP_CONTEXT);
                add("JOIN_GAME_SETUP_CONTEXT", (int32_t)Blaze::GameManager::JOIN_GAME_SETUP_CONTEXT);
                add("INDIRECT_JOIN_GAME_FROM_QUEUE_SETUP_CONTEXT", (int32_t)Blaze::GameManager::INDIRECT_JOIN_GAME_FROM_QUEUE_SETUP_CONTEXT);
                add("INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT", (int32_t)Blaze::GameManager::INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT);
                add("HOST_INJECTION_SETUP_CONTEXT", (int32_t)Blaze::GameManager::HOST_INJECTION_SETUP_CONTEXT);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::UpdateAdminListOperation> : public EnumNameValueCollectionT<Blaze::GameManager::UpdateAdminListOperation>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::UpdateAdminListOperation> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::UpdateAdminListOperation> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::UpdateAdminListOperation>("Blaze::GameManager::UpdateAdminListOperation")
            {
                add("GM_ADMIN_ADDED", (int32_t)Blaze::GameManager::GM_ADMIN_ADDED);
                add("GM_ADMIN_REMOVED", (int32_t)Blaze::GameManager::GM_ADMIN_REMOVED);
                add("GM_ADMIN_MIGRATED", (int32_t)Blaze::GameManager::GM_ADMIN_MIGRATED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::GameManagerCensusEnum> : public EnumNameValueCollectionT<Blaze::GameManager::GameManagerCensusEnum>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::GameManagerCensusEnum> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::GameManagerCensusEnum> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::GameManagerCensusEnum>("Blaze::GameManager::GameManagerCensusEnum")
            {
                add("GM_NUM_PLAYER_ONLINE", (int32_t)Blaze::GameManager::GM_NUM_PLAYER_ONLINE);
                add("GM_NUM_ACTIVE_GAME", (int32_t)Blaze::GameManager::GM_NUM_ACTIVE_GAME);
                add("GM_NUM_PLAYER_IN_GAME", (int32_t)Blaze::GameManager::GM_NUM_PLAYER_IN_GAME);
                add("GM_NUM_PLAYER_IN_MATCHMAKING", (int32_t)Blaze::GameManager::GM_NUM_PLAYER_IN_MATCHMAKING);
                add("GM_GAME_ATTRIBUTE_START", (int32_t)Blaze::GameManager::GM_GAME_ATTRIBUTE_START);
                add("GM_GAME_ATTRIBUTE_END", (int32_t)Blaze::GameManager::GM_GAME_ATTRIBUTE_END);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::MatchmakingResult> : public EnumNameValueCollectionT<Blaze::GameManager::MatchmakingResult>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::MatchmakingResult> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::MatchmakingResult> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameManager::MatchmakingResult>("Blaze::GameManager::MatchmakingResult")
            {
                add("SUCCESS_CREATED_GAME", (int32_t)Blaze::GameManager::SUCCESS_CREATED_GAME);
                add("SUCCESS_JOINED_NEW_GAME", (int32_t)Blaze::GameManager::SUCCESS_JOINED_NEW_GAME);
                add("SUCCESS_JOINED_EXISTING_GAME", (int32_t)Blaze::GameManager::SUCCESS_JOINED_EXISTING_GAME);
                add("SESSION_TIMED_OUT", (int32_t)Blaze::GameManager::SESSION_TIMED_OUT);
                add("SESSION_CANCELED", (int32_t)Blaze::GameManager::SESSION_CANCELED);
                add("SESSION_TERMINATED", (int32_t)Blaze::GameManager::SESSION_TERMINATED);
                add("SESSION_ERROR_GAME_SETUP_FAILED", (int32_t)Blaze::GameManager::SESSION_ERROR_GAME_SETUP_FAILED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Stats::StatPeriodType> : public EnumNameValueCollectionT<Blaze::Stats::StatPeriodType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Stats::StatPeriodType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Stats::StatPeriodType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Stats::StatPeriodType>("Blaze::Stats::StatPeriodType")
            {
                add("STAT_PERIOD_ALL_TIME", (int32_t)Blaze::Stats::STAT_PERIOD_ALL_TIME);
                add("STAT_PERIOD_MONTHLY", (int32_t)Blaze::Stats::STAT_PERIOD_MONTHLY);
                add("STAT_PERIOD_WEEKLY", (int32_t)Blaze::Stats::STAT_PERIOD_WEEKLY);
                add("STAT_PERIOD_DAILY", (int32_t)Blaze::Stats::STAT_PERIOD_DAILY);
                add("STAT_NUM_PERIODS", (int32_t)Blaze::Stats::STAT_NUM_PERIODS);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Stats::CategoryType> : public EnumNameValueCollectionT<Blaze::Stats::CategoryType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Stats::CategoryType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Stats::CategoryType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Stats::CategoryType>("Blaze::Stats::CategoryType")
            {
                add("CATEGORY_TYPE_REGULAR", (int32_t)Blaze::Stats::CATEGORY_TYPE_REGULAR);
                add("CATEGORY_TYPE_GLOBAL", (int32_t)Blaze::Stats::CATEGORY_TYPE_GLOBAL);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::ClubsOrder> : public EnumNameValueCollectionT<Blaze::Clubs::ClubsOrder>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::ClubsOrder> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::ClubsOrder> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::ClubsOrder>("Blaze::Clubs::ClubsOrder")
            {
                add("CLUBS_NO_ORDER", (int32_t)Blaze::Clubs::CLUBS_NO_ORDER);
                add("CLUBS_ORDER_BY_NAME", (int32_t)Blaze::Clubs::CLUBS_ORDER_BY_NAME);
                add("CLUBS_ORDER_BY_CREATIONTIME", (int32_t)Blaze::Clubs::CLUBS_ORDER_BY_CREATIONTIME);
                add("CLUBS_ORDER_BY_MEMBERCOUNT", (int32_t)Blaze::Clubs::CLUBS_ORDER_BY_MEMBERCOUNT);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::OrderMode> : public EnumNameValueCollectionT<Blaze::Clubs::OrderMode>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::OrderMode> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::OrderMode> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::OrderMode>("Blaze::Clubs::OrderMode")
            {
                add("ASC_ORDER", (int32_t)Blaze::Clubs::ASC_ORDER);
                add("DESC_ORDER", (int32_t)Blaze::Clubs::DESC_ORDER);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::MemberOrder> : public EnumNameValueCollectionT<Blaze::Clubs::MemberOrder>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::MemberOrder> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::MemberOrder> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::MemberOrder>("Blaze::Clubs::MemberOrder")
            {
                add("MEMBER_NO_ORDER", (int32_t)Blaze::Clubs::MEMBER_NO_ORDER);
                add("MEMBER_ORDER_BY_JOIN_TIME", (int32_t)Blaze::Clubs::MEMBER_ORDER_BY_JOIN_TIME);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::MemberTypeFilter> : public EnumNameValueCollectionT<Blaze::Clubs::MemberTypeFilter>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::MemberTypeFilter> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::MemberTypeFilter> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::MemberTypeFilter>("Blaze::Clubs::MemberTypeFilter")
            {
                add("ALL_MEMBERS", (int32_t)Blaze::Clubs::ALL_MEMBERS);
                add("GM_MEMBERS", (int32_t)Blaze::Clubs::GM_MEMBERS);
                add("NON_GM_MEMBERS", (int32_t)Blaze::Clubs::NON_GM_MEMBERS);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::SearchRequestorAcceptance> : public EnumNameValueCollectionT<Blaze::Clubs::SearchRequestorAcceptance>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::SearchRequestorAcceptance> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::SearchRequestorAcceptance> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::SearchRequestorAcceptance>("Blaze::Clubs::SearchRequestorAcceptance")
            {
                add("CLUB_ACCEPTS_UNSPECIFIED", (int32_t)Blaze::Clubs::CLUB_ACCEPTS_UNSPECIFIED);
                add("CLUB_ACCEPTS_ALL", (int32_t)Blaze::Clubs::CLUB_ACCEPTS_ALL);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::BanStatusValues> : public EnumNameValueCollectionT<Blaze::Clubs::BanStatusValues>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::BanStatusValues> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::BanStatusValues> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::BanStatusValues>("Blaze::Clubs::BanStatusValues")
            {
                add("CLUBS_NO_BAN", (int32_t)Blaze::Clubs::CLUBS_NO_BAN);
                add("CLUBS_BAN_BY_GM", (int32_t)Blaze::Clubs::CLUBS_BAN_BY_GM);
                add("CLUBS_BAN_BY_ADMIN", (int32_t)Blaze::Clubs::CLUBS_BAN_BY_ADMIN);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::ClubsCensusEnum> : public EnumNameValueCollectionT<Blaze::Clubs::ClubsCensusEnum>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::ClubsCensusEnum> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::ClubsCensusEnum> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::ClubsCensusEnum>("Blaze::Clubs::ClubsCensusEnum")
            {
                add("CLUBS_MEMBERS_DOMAIN", (int32_t)Blaze::Clubs::CLUBS_MEMBERS_DOMAIN);
                add("CLUBS_ONLINE_MEMBERS_DOMAIN", (int32_t)Blaze::Clubs::CLUBS_ONLINE_MEMBERS_DOMAIN);
                add("CLUBS_DOMAIN", (int32_t)Blaze::Clubs::CLUBS_DOMAIN);
                add("CLUBS_ONLINE_DOMAIN", (int32_t)Blaze::Clubs::CLUBS_ONLINE_DOMAIN);
                add("CLUBS_MEMBERS", (int32_t)Blaze::Clubs::CLUBS_MEMBERS);
                add("CLUBS_ONLINE_MEMBERS", (int32_t)Blaze::Clubs::CLUBS_ONLINE_MEMBERS);
                add("CLUBS", (int32_t)Blaze::Clubs::CLUBS);
                add("CLUBS_ONLINE", (int32_t)Blaze::Clubs::CLUBS_ONLINE);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::NewsParamType> : public EnumNameValueCollectionT<Blaze::Clubs::NewsParamType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::NewsParamType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::NewsParamType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::NewsParamType>("Blaze::Clubs::NewsParamType")
            {
                add("NEWS_PARAM_NONE", (int32_t)Blaze::Clubs::NEWS_PARAM_NONE);
                add("NEWS_PARAM_STRING", (int32_t)Blaze::Clubs::NEWS_PARAM_STRING);
                add("NEWS_PARAM_INT", (int32_t)Blaze::Clubs::NEWS_PARAM_INT);
                add("NEWS_PARAM_PLAYER_ID", (int32_t)Blaze::Clubs::NEWS_PARAM_PLAYER_ID);
                add("NEWS_PARAM_TROPHY_ID", (int32_t)Blaze::Clubs::NEWS_PARAM_TROPHY_ID);
                add("NEWS_PARAM_BLAZE_ID", (int32_t)Blaze::Clubs::NEWS_PARAM_BLAZE_ID);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::ClubConfigParams> : public EnumNameValueCollectionT<Blaze::Clubs::ClubConfigParams>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::ClubConfigParams> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::ClubConfigParams> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::ClubConfigParams>("Blaze::Clubs::ClubConfigParams")
            {
                add("CLUBS_MAX_MEMBER_COUNT", (int32_t)Blaze::Clubs::CLUBS_MAX_MEMBER_COUNT);
                add("CLUBS_MAX_GM_COUNT", (int32_t)Blaze::Clubs::CLUBS_MAX_GM_COUNT);
                add("CLUBS_MAX_DIVISION_SIZE", (int32_t)Blaze::Clubs::CLUBS_MAX_DIVISION_SIZE);
                add("CLUBS_MAX_NEWS_COUNT", (int32_t)Blaze::Clubs::CLUBS_MAX_NEWS_COUNT);
                add("CLUBS_MAX_INVITE_COUNT", (int32_t)Blaze::Clubs::CLUBS_MAX_INVITE_COUNT);
                add("CLUBS_MAX_INACTIVE_DAYS", (int32_t)Blaze::Clubs::CLUBS_MAX_INACTIVE_DAYS);
                add("CLUBS_PURGE_HOUR", (int32_t)Blaze::Clubs::CLUBS_PURGE_HOUR);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::MessageType> : public EnumNameValueCollectionT<Blaze::Clubs::MessageType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::MessageType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::MessageType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::MessageType>("Blaze::Clubs::MessageType")
            {
                add("CLUBS_INVITATION", (int32_t)Blaze::Clubs::CLUBS_INVITATION);
                add("CLUBS_PETITION", (int32_t)Blaze::Clubs::CLUBS_PETITION);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::InvitationsType> : public EnumNameValueCollectionT<Blaze::Clubs::InvitationsType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::InvitationsType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::InvitationsType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::InvitationsType>("Blaze::Clubs::InvitationsType")
            {
                add("CLUBS_SENT_TO_ME", (int32_t)Blaze::Clubs::CLUBS_SENT_TO_ME);
                add("CLUBS_SENT_BY_ME", (int32_t)Blaze::Clubs::CLUBS_SENT_BY_ME);
                add("CLUBS_SENT_BY_CLUB", (int32_t)Blaze::Clubs::CLUBS_SENT_BY_CLUB);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::PetitionsType> : public EnumNameValueCollectionT<Blaze::Clubs::PetitionsType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::PetitionsType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::PetitionsType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::PetitionsType>("Blaze::Clubs::PetitionsType")
            {
                add("CLUBS_PETITION_SENT_BY_ME", (int32_t)Blaze::Clubs::CLUBS_PETITION_SENT_BY_ME);
                add("CLUBS_PETITION_SENT_TO_CLUB", (int32_t)Blaze::Clubs::CLUBS_PETITION_SENT_TO_CLUB);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::NewsType> : public EnumNameValueCollectionT<Blaze::Clubs::NewsType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::NewsType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::NewsType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::NewsType>("Blaze::Clubs::NewsType")
            {
                add("CLUBS_SERVER_GENERATED_NEWS", (int32_t)Blaze::Clubs::CLUBS_SERVER_GENERATED_NEWS);
                add("CLUBS_MEMBER_POSTED_NEWS", (int32_t)Blaze::Clubs::CLUBS_MEMBER_POSTED_NEWS);
                add("CLUBS_ASSOCIATE_NEWS", (int32_t)Blaze::Clubs::CLUBS_ASSOCIATE_NEWS);
                add("CLUBS_ALL_NEWS", (int32_t)Blaze::Clubs::CLUBS_ALL_NEWS);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::TimeSortType> : public EnumNameValueCollectionT<Blaze::Clubs::TimeSortType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::TimeSortType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::TimeSortType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::TimeSortType>("Blaze::Clubs::TimeSortType")
            {
                add("CLUBS_SORT_TIME_DESC", (int32_t)Blaze::Clubs::CLUBS_SORT_TIME_DESC);
                add("CLUBS_SORT_TIME_ASC", (int32_t)Blaze::Clubs::CLUBS_SORT_TIME_ASC);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::MetaDataType> : public EnumNameValueCollectionT<Blaze::Clubs::MetaDataType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::MetaDataType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::MetaDataType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::MetaDataType>("Blaze::Clubs::MetaDataType")
            {
                add("CLUBS_METADATA_TYPE_STRING", (int32_t)Blaze::Clubs::CLUBS_METADATA_TYPE_STRING);
                add("CLUBS_METADATA_TYPE_BINARY", (int32_t)Blaze::Clubs::CLUBS_METADATA_TYPE_BINARY);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::MembershipStatus> : public EnumNameValueCollectionT<Blaze::Clubs::MembershipStatus>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::MembershipStatus> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::MembershipStatus> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::MembershipStatus>("Blaze::Clubs::MembershipStatus")
            {
                add("CLUBS_MEMBER", (int32_t)Blaze::Clubs::CLUBS_MEMBER);
                add("CLUBS_GM", (int32_t)Blaze::Clubs::CLUBS_GM);
                add("CLUBS_OWNER", (int32_t)Blaze::Clubs::CLUBS_OWNER);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::MemberOnlineStatus> : public EnumNameValueCollectionT<Blaze::Clubs::MemberOnlineStatus>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::MemberOnlineStatus> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::MemberOnlineStatus> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::MemberOnlineStatus>("Blaze::Clubs::MemberOnlineStatus")
            {
                add("CLUBS_MEMBER_OFFLINE", (int32_t)Blaze::Clubs::CLUBS_MEMBER_OFFLINE);
                add("CLUBS_MEMBER_ONLINE_INTERACTIVE", (int32_t)Blaze::Clubs::CLUBS_MEMBER_ONLINE_INTERACTIVE);
                add("CLUBS_MEMBER_ONLINE_NON_INTERACTIVE", (int32_t)Blaze::Clubs::CLUBS_MEMBER_ONLINE_NON_INTERACTIVE);
                add("CLUBS_MEMBER_IN_GAMEROOM", (int32_t)Blaze::Clubs::CLUBS_MEMBER_IN_GAMEROOM);
                add("CLUBS_MEMBER_IN_OPEN_SESSION", (int32_t)Blaze::Clubs::CLUBS_MEMBER_IN_OPEN_SESSION);
                add("CLUBS_MEMBER_IN_GAME", (int32_t)Blaze::Clubs::CLUBS_MEMBER_IN_GAME);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_1", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_1);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_2", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_2);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_3", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_3);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_4", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_4);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_5", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_5);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_6", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_6);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_7", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_7);
                add("CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_8", (int32_t)Blaze::Clubs::CLUBS_MEMBER_CUSTOM_ONLINE_STATUS_8);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::UpdateReason> : public EnumNameValueCollectionT<Blaze::Clubs::UpdateReason>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::UpdateReason> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::UpdateReason> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::UpdateReason>("Blaze::Clubs::UpdateReason")
            {
                add("UPDATE_REASON_USER_SESSION_CREATED", (int32_t)Blaze::Clubs::UPDATE_REASON_USER_SESSION_CREATED);
                add("UPDATE_REASON_USER_SESSION_DESTROYED", (int32_t)Blaze::Clubs::UPDATE_REASON_USER_SESSION_DESTROYED);
                add("UPDATE_REASON_USER_JOINED_CLUB", (int32_t)Blaze::Clubs::UPDATE_REASON_USER_JOINED_CLUB);
                add("UPDATE_REASON_USER_LEFT_CLUB", (int32_t)Blaze::Clubs::UPDATE_REASON_USER_LEFT_CLUB);
                add("UPDATE_REASON_USER_ONLINE_STATUS_UPDATED", (int32_t)Blaze::Clubs::UPDATE_REASON_USER_ONLINE_STATUS_UPDATED);
                add("UPDATE_REASON_CLUB_DESTROYED", (int32_t)Blaze::Clubs::UPDATE_REASON_CLUB_DESTROYED);
                add("UPDATE_REASON_CLUB_CREATED", (int32_t)Blaze::Clubs::UPDATE_REASON_CLUB_CREATED);
                add("UPDATE_REASON_CLUB_SETTINGS_UPDATED", (int32_t)Blaze::Clubs::UPDATE_REASON_CLUB_SETTINGS_UPDATED);
                add("UPDATE_REASON_USER_PROMOTED_TO_GM", (int32_t)Blaze::Clubs::UPDATE_REASON_USER_PROMOTED_TO_GM);
                add("UPDATE_REASON_USER_DEMOTED_TO_MEMBER", (int32_t)Blaze::Clubs::UPDATE_REASON_USER_DEMOTED_TO_MEMBER);
                add("UPDATE_REASON_TRANSFER_OWNERSHIP", (int32_t)Blaze::Clubs::UPDATE_REASON_TRANSFER_OWNERSHIP);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::ArtPackageType> : public EnumNameValueCollectionT<Blaze::Clubs::ArtPackageType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::ArtPackageType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::ArtPackageType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::ArtPackageType>("Blaze::Clubs::ArtPackageType")
            {
                add("CLUBS_ART_UNKNOWN", (int32_t)Blaze::Clubs::CLUBS_ART_UNKNOWN);
                add("CLUBS_ART_JPG", (int32_t)Blaze::Clubs::CLUBS_ART_JPG);
                add("CLUBS_ART_GIF", (int32_t)Blaze::Clubs::CLUBS_ART_GIF);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::RecordStatType> : public EnumNameValueCollectionT<Blaze::Clubs::RecordStatType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::RecordStatType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::RecordStatType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::RecordStatType>("Blaze::Clubs::RecordStatType")
            {
                add("CLUBS_RECORD_STAT_INT", (int32_t)Blaze::Clubs::CLUBS_RECORD_STAT_INT);
                add("CLUBS_RECORD_STAT_FLOAT", (int32_t)Blaze::Clubs::CLUBS_RECORD_STAT_FLOAT);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::SeasonPeriod> : public EnumNameValueCollectionT<Blaze::Clubs::SeasonPeriod>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::SeasonPeriod> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::SeasonPeriod> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::SeasonPeriod>("Blaze::Clubs::SeasonPeriod")
            {
                add("CLUBS_PERIOD_DAILY", (int32_t)Blaze::Clubs::CLUBS_PERIOD_DAILY);
                add("CLUBS_PERIOD_WEEKLY", (int32_t)Blaze::Clubs::CLUBS_PERIOD_WEEKLY);
                add("CLUBS_PERIOD_MONTHLY", (int32_t)Blaze::Clubs::CLUBS_PERIOD_MONTHLY);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::SeasonState> : public EnumNameValueCollectionT<Blaze::Clubs::SeasonState>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::SeasonState> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::SeasonState> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::SeasonState>("Blaze::Clubs::SeasonState")
            {
                add("CLUBS_IN_SEASON", (int32_t)Blaze::Clubs::CLUBS_IN_SEASON);
                add("CLUBS_POST_SEASON", (int32_t)Blaze::Clubs::CLUBS_POST_SEASON);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::TagSearchOperation> : public EnumNameValueCollectionT<Blaze::Clubs::TagSearchOperation>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::TagSearchOperation> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::TagSearchOperation> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::TagSearchOperation>("Blaze::Clubs::TagSearchOperation")
            {
                add("CLUB_TAG_SEARCH_IGNORE", (int32_t)Blaze::Clubs::CLUB_TAG_SEARCH_IGNORE);
                add("CLUB_TAG_SEARCH_MATCH_ANY", (int32_t)Blaze::Clubs::CLUB_TAG_SEARCH_MATCH_ANY);
                add("CLUB_TAG_SEARCH_MATCH_ALL", (int32_t)Blaze::Clubs::CLUB_TAG_SEARCH_MATCH_ALL);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::PasswordOption> : public EnumNameValueCollectionT<Blaze::Clubs::PasswordOption>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::PasswordOption> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::PasswordOption> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::PasswordOption>("Blaze::Clubs::PasswordOption")
            {
                add("CLUB_PASSWORD_IGNORE", (int32_t)Blaze::Clubs::CLUB_PASSWORD_IGNORE);
                add("CLUB_PASSWORD_NOT_PROTECTED", (int32_t)Blaze::Clubs::CLUB_PASSWORD_NOT_PROTECTED);
                add("CLUB_PASSWORD_PROTECTED", (int32_t)Blaze::Clubs::CLUB_PASSWORD_PROTECTED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::ClubLogEventKeyType> : public EnumNameValueCollectionT<Blaze::Clubs::ClubLogEventKeyType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::ClubLogEventKeyType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::ClubLogEventKeyType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::ClubLogEventKeyType>("Blaze::Clubs::ClubLogEventKeyType")
            {
                add("CLUB_LOG_EVENT_CREATE_CLUB", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_CREATE_CLUB);
                add("CLUB_LOG_EVENT_JOIN_CLUB", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_JOIN_CLUB);
                add("CLUB_LOG_EVENT_LEAVE_CLUB", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_LEAVE_CLUB);
                add("CLUB_LOG_EVENT_REMOVED_FROM_CLUB", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_REMOVED_FROM_CLUB);
                add("CLUB_LOG_EVENT_AWARD_TROPHY", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_AWARD_TROPHY);
                add("CLUB_LOG_EVENT_GM_PROMOTED", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_GM_PROMOTED);
                add("CLUB_LOG_EVENT_GM_DEMOTED", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_GM_DEMOTED);
                add("CLUB_LOG_EVENT_OWNERSHIP_TRANSFERRED", (int32_t)Blaze::Clubs::CLUB_LOG_EVENT_OWNERSHIP_TRANSFERRED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::RequestorAcceptance> : public EnumNameValueCollectionT<Blaze::Clubs::RequestorAcceptance>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::RequestorAcceptance> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::RequestorAcceptance> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::RequestorAcceptance>("Blaze::Clubs::RequestorAcceptance")
            {
                add("CLUB_ACCEPT_NONE", (int32_t)Blaze::Clubs::CLUB_ACCEPT_NONE);
                add("CLUB_ACCEPT_GM_FRIENDS", (int32_t)Blaze::Clubs::CLUB_ACCEPT_GM_FRIENDS);
                add("CLUB_ACCEPT_ALL", (int32_t)Blaze::Clubs::CLUB_ACCEPT_ALL);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::ClubJoinOrPetitionStatus> : public EnumNameValueCollectionT<Blaze::Clubs::ClubJoinOrPetitionStatus>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::ClubJoinOrPetitionStatus> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::ClubJoinOrPetitionStatus> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::ClubJoinOrPetitionStatus>("Blaze::Clubs::ClubJoinOrPetitionStatus")
            {
                add("CLUB_JOIN_AND_PETITION_FAILURE", (int32_t)Blaze::Clubs::CLUB_JOIN_AND_PETITION_FAILURE);
                add("CLUB_JOIN_SUCCESS", (int32_t)Blaze::Clubs::CLUB_JOIN_SUCCESS);
                add("CLUB_PETITION_SUCCESS", (int32_t)Blaze::Clubs::CLUB_PETITION_SUCCESS);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::PetitionMessageType> : public EnumNameValueCollectionT<Blaze::Clubs::PetitionMessageType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::PetitionMessageType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::PetitionMessageType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::PetitionMessageType>("Blaze::Clubs::PetitionMessageType")
            {
                add("CLUBS_PI_PETITION_SENT", (int32_t)Blaze::Clubs::CLUBS_PI_PETITION_SENT);
                add("CLUBS_PI_PETITION_ACCEPTED", (int32_t)Blaze::Clubs::CLUBS_PI_PETITION_ACCEPTED);
                add("CLUBS_PI_PETITION_DECLINED", (int32_t)Blaze::Clubs::CLUBS_PI_PETITION_DECLINED);
                add("CLUBS_PI_PETITION_REVOKED", (int32_t)Blaze::Clubs::CLUBS_PI_PETITION_REVOKED);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Clubs::PetitionMessageAttribute> : public EnumNameValueCollectionT<Blaze::Clubs::PetitionMessageAttribute>
    {
        public:
            static const PyroCustomEnumValues<Blaze::Clubs::PetitionMessageAttribute> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::Clubs::PetitionMessageAttribute> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::Clubs::PetitionMessageAttribute>("Blaze::Clubs::PetitionMessageAttribute")
            {
                add("CLUBS_MSGATR_MESSAGE_TYPE", (int32_t)Blaze::Clubs::CLUBS_MSGATR_MESSAGE_TYPE);
                add("CLUBS_MSGATR_CLUB_MSG_ID", (int32_t)Blaze::Clubs::CLUBS_MSGATR_CLUB_MSG_ID);
                add("CLUBS_MSGATR_CLUB_NAME", (int32_t)Blaze::Clubs::CLUBS_MSGATR_CLUB_NAME);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::GameBrowserList::ListType> : public EnumNameValueCollectionT<Blaze::GameManager::GameBrowserList::ListType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::GameBrowserList::ListType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::GameBrowserList::ListType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues() : EnumNameValueCollectionT<Blaze::GameManager::GameBrowserList::ListType>("Blaze::GameManager::GameBrowserList::ListType")
            {
                add("LIST_TYPE_SNAPSHOT", (int32_t)Blaze::GameManager::GameBrowserList::LIST_TYPE_SNAPSHOT);
                add("LIST_TYPE_SUBSCRIPTION", (int32_t)Blaze::GameManager::GameBrowserList::LIST_TYPE_SUBSCRIPTION);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::UserManager::User::OnlineStatus> : public EnumNameValueCollectionT<Blaze::UserManager::User::OnlineStatus>
    {
        public:
            static const PyroCustomEnumValues<Blaze::UserManager::User::OnlineStatus> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::UserManager::User::OnlineStatus> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues() : EnumNameValueCollectionT<Blaze::UserManager::User::OnlineStatus>("Blaze::UserManager::User::OnlineStatus")
            {
                add("UNKNOWN", (int32_t)Blaze::UserManager::User::UNKNOWN);
                add("OFFLINE", (int32_t)Blaze::UserManager::User::OFFLINE);
                add("ONLINE", (int32_t)Blaze::UserManager::User::ONLINE);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameManager::HostViabilityRuleStatus::HostViabilityValues> : public EnumNameValueCollectionT<Blaze::GameManager::HostViabilityRuleStatus::HostViabilityValues>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameManager::HostViabilityRuleStatus::HostViabilityValues> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameManager::HostViabilityRuleStatus::HostViabilityValues> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues() : EnumNameValueCollectionT<Blaze::GameManager::HostViabilityRuleStatus::HostViabilityValues>("Blaze::GameManager::HostViabilityRuleStatus::HostViabilityValues")
            {
                add("CONNECTION_ASSURED", (int32_t)Blaze::GameManager::HostViabilityRuleStatus::CONNECTION_ASSURED);
                add("CONNECTION_LIKELY", (int32_t)Blaze::GameManager::HostViabilityRuleStatus::CONNECTION_LIKELY);
                add("CONNECTION_FEASIBLE", (int32_t)Blaze::GameManager::HostViabilityRuleStatus::CONNECTION_FEASIBLE);
                add("CONNECTION_UNLIKELY", (int32_t)Blaze::GameManager::HostViabilityRuleStatus::CONNECTION_UNLIKELY);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::GameReporting::GameReportPlayerFinishedStatus> : public EnumNameValueCollectionT<Blaze::GameReporting::GameReportPlayerFinishedStatus>
    {
        public:
            static const PyroCustomEnumValues<Blaze::GameReporting::GameReportPlayerFinishedStatus> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::GameReporting::GameReportPlayerFinishedStatus> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::GameReporting::GameReportPlayerFinishedStatus>("Blaze::GameReporting::GameReportPlayerFinishedStatus")
            {
                add("GAMEREPORT_FINISHED_STATUS_DEFAULT", (int32_t)Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DEFAULT);
                add("GAMEREPORT_FINISHED_STATUS_FINISHED", (int32_t)Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_FINISHED);
                add("GAMEREPORT_FINISHED_STATUS_DNF", (int32_t)Blaze::GameReporting::GAMEREPORT_FINISHED_STATUS_DNF);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::LocalizeType> : public EnumNameValueCollectionT<Blaze::LocalizeType>
    {
        public:
            static const PyroCustomEnumValues<Blaze::LocalizeType> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::LocalizeType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::LocalizeType>("Blaze::LocalizeType")
            {
                add("LOCALIZE_TYPE_PERCENT", (int32_t)Blaze::LOCALIZE_TYPE_PERCENT);
                add("LOCALIZE_TYPE_NUMBER", (int32_t)Blaze::LOCALIZE_TYPE_NUMBER);
                add("LOCALIZE_TYPE_SEPNUM", (int32_t)Blaze::LOCALIZE_TYPE_SEPNUM);
                add("LOCALIZE_TYPE_TIME", (int32_t)Blaze::LOCALIZE_TYPE_TIME);
                add("LOCALIZE_TYPE_DATE", (int32_t)Blaze::LOCALIZE_TYPE_DATE);
                add("LOCALIZE_TYPE_DATETIME", (int32_t)Blaze::LOCALIZE_TYPE_DATETIME);
                add("LOCALIZE_TYPE_STRING", (int32_t)Blaze::LOCALIZE_TYPE_STRING);
                add("LOCALIZE_TYPE_RAW", (int32_t)Blaze::LOCALIZE_TYPE_RAW);
                add("LOCALIZE_TYPE_CURRENCY", (int32_t)Blaze::LOCALIZE_TYPE_CURRENCY);
                add("LOCALIZE_TYPE_RANK", (int32_t)Blaze::LOCALIZE_TYPE_RANK);
                add("LOCALIZE_TYPE_RANK_PTS", (int32_t)Blaze::LOCALIZE_TYPE_RANK_PTS);
                add("LOCALIZE_TYPE_NAME", (int32_t)Blaze::LOCALIZE_TYPE_NAME);
            }
    };

    template<>
    class PyroCustomEnumValues<Blaze::ByteVault::AdminType> : public EnumNameValueCollectionT<Blaze::ByteVault::AdminType>
    {
    public:
        static const PyroCustomEnumValues<Blaze::ByteVault::AdminType> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::ByteVault::AdminType> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::ByteVault::AdminType>("Blaze::ByteVault::AdminType")
        {
            add("ADMIN_TYPE_INVALID", (int32_t)Blaze::ByteVault::ADMIN_TYPE_INVALID);
            add("ADMIN_TYPE_SERVICE", (int32_t)Blaze::ByteVault::ADMIN_TYPE_SERVICE);
            add("ADMIN_TYPE_DATA", (int32_t)Blaze::ByteVault::ADMIN_TYPE_DATA);
            add("ADMIN_TYPE_CONTEXT_DEVELOPMENT", (int32_t)Blaze::ByteVault::ADMIN_TYPE_CONTEXT_DEVELOPMENT);
            add("ADMIN_TYPE_CONTEXT_DATA", (int32_t)Blaze::ByteVault::ADMIN_TYPE_CONTEXT_DATA);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::ByteVault::UserType> : public EnumNameValueCollectionT<Blaze::ByteVault::UserType>
    {
    public:
        static const PyroCustomEnumValues<Blaze::ByteVault::UserType> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::ByteVault::UserType> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::ByteVault::UserType>("Blaze::ByteVault::UserType")
        {
            add("USER_TYPE_INVALID", (int32_t)Blaze::ByteVault::USER_TYPE_INVALID);
            add("NUCLEUS_USER", (int32_t)Blaze::ByteVault::NUCLEUS_USER);
            add("NUCLEUS_PERSONA", (int32_t)Blaze::ByteVault::NUCLEUS_PERSONA);
            add("GLOBAL", (int32_t)Blaze::ByteVault::GLOBAL);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::ByteVault::TokenType> : public EnumNameValueCollectionT<Blaze::ByteVault::TokenType>
    {
    public:
        static const PyroCustomEnumValues<Blaze::ByteVault::TokenType> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::ByteVault::TokenType> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::ByteVault::TokenType>("Blaze::ByteVault::TokenType")
        {
            add("INVALID_TOKEN_TYPE", (int32_t)Blaze::ByteVault::INVALID_TOKEN_TYPE);
            add("NUCLEUS_AUTH_TOKEN", (int32_t)Blaze::ByteVault::NUCLEUS_AUTH_TOKEN);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::LocalizeError> : public EnumNameValueCollectionT<Blaze::LocalizeError>
    {
        public:
            static const PyroCustomEnumValues<Blaze::LocalizeError> &getInstance()
            {
                static PyroCustomEnumValues<Blaze::LocalizeError> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues()
                : EnumNameValueCollectionT<Blaze::LocalizeError>("Blaze::LocalizeError")
            {
                add("LOCALIZE_ERROR_NONE", (int32_t)Blaze::LOCALIZE_ERROR_NONE);
                add("LOCALIZE_ERROR_LANG", (int32_t)Blaze::LOCALIZE_ERROR_LANG);
                add("LOCALIZE_ERROR_COUNTRY", (int32_t)Blaze::LOCALIZE_ERROR_COUNTRY);
                add("LOCALIZE_ERROR_LANG_COUNTRY", (int32_t)Blaze::LOCALIZE_ERROR_LANG_COUNTRY);
                add("LOCALIZE_ERROR_TYPE", (int32_t)Blaze::LOCALIZE_ERROR_TYPE);
                add("LOCALIZE_ERROR_BUFFER", (int32_t)Blaze::LOCALIZE_ERROR_BUFFER);
                add("LOCALIZE_ERROR_DATA", (int32_t)Blaze::LOCALIZE_ERROR_DATA);
                add("LOCALIZE_ERROR_MAXSEC", (int32_t)Blaze::LOCALIZE_ERROR_MAXSEC);
                add("LOCALIZE_ERROR_FATAL", (int32_t)Blaze::LOCALIZE_ERROR_FATAL);
            }
    };

    template<>
    class PyroCustomEnumValues<Ignition::MatchmakingSessionType> : public EnumNameValueCollectionT<Ignition::MatchmakingSessionType>
    {
        public:
            static const PyroCustomEnumValues<Ignition::MatchmakingSessionType> &getInstance()
            {
                static PyroCustomEnumValues<Ignition::MatchmakingSessionType> instance;
                return instance;
            }

        private:
            PyroCustomEnumValues() : EnumNameValueCollectionT<Ignition::MatchmakingSessionType>("Ignition::MatchmakingSessionType")
            {
                add("FIND", (int32_t)Ignition::FIND);
                add("CREATE", (int32_t)Ignition::CREATE);
                add("FIND_OR_CREATE", (int32_t)Ignition::FIND_OR_CREATE);
            }
    };

    template<>
    class PyroCustomEnumValues<Ignition::PlayerCapacityChangeTeamSetup> : public EnumNameValueCollectionT<Ignition::PlayerCapacityChangeTeamSetup>
    {
    public:
        static const PyroCustomEnumValues<Ignition::PlayerCapacityChangeTeamSetup> &getInstance()
        {
            static PyroCustomEnumValues<Ignition::PlayerCapacityChangeTeamSetup> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues() : EnumNameValueCollectionT<Ignition::PlayerCapacityChangeTeamSetup>("Ignition::PlayerCapacityChangeTeamSetup")
        {
            add("Assign local connection group to one team.", (int32_t)Ignition::ASSIGN_LOCAL_CONNECTION_GROUP_TO_SAME_TEAM);
            add("Distribute local connection group across teams.", (int32_t)Ignition::SEPERATE_LOCAL_CONNECTION_GROUP);
            add("Make no team assignments.", (int32_t)Ignition::NO_TEAM_ASSIGNMENTS);
        }
    };

    template<>
    class PyroCustomEnumValues<Ignition::CreateGameTeamAndRoleSetup> : public EnumNameValueCollectionT<Ignition::CreateGameTeamAndRoleSetup>
    {
    public:
        static const PyroCustomEnumValues<Ignition::CreateGameTeamAndRoleSetup> &getInstance()
        {
            static PyroCustomEnumValues<Ignition::CreateGameTeamAndRoleSetup> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues() : EnumNameValueCollectionT<Ignition::CreateGameTeamAndRoleSetup>("Ignition::CreateGameTeamAndRoleSetup")
        {
            add("Default role for everyone.", (int32_t)Ignition::DEFAULT_ROLES);
            add("Simple roles (only forward and defender roles)", (int32_t)Ignition::SIMPLE_ROLES);
            add("Teams with defenders, midfielders, forwards and goalies. (Limits are 5-5-4 + goalie, regardless of team size)", (int32_t)Ignition::FIFA_ROLES);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Achievements::UserType> : public EnumNameValueCollectionT<Blaze::Achievements::UserType>
    {
    public:
        static const PyroCustomEnumValues<Blaze::Achievements::UserType> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::Achievements::UserType> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::Achievements::UserType>("Blaze::Achievements::UserType")
        {
            add("USER_TYPE_INVALID", (int32_t)Blaze::Achievements::USER_TYPE_INVALID);
            add("USER", (int32_t)Blaze::Achievements::USER);
            add("PERSONA", (int32_t)Blaze::Achievements::PERSONA);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Achievements::FilterType> : public EnumNameValueCollectionT<Blaze::Achievements::FilterType>
    {
    public:
        static const PyroCustomEnumValues<Blaze::Achievements::FilterType> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::Achievements::FilterType> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::Achievements::FilterType>("Blaze::Achievements::FilterType")
        {
            add("ALL", (int32_t)Blaze::Achievements::ALL);
            add("ACTIVE", (int32_t)Blaze::Achievements::ACTIVE);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::ClientState::Status> : public EnumNameValueCollectionT<Blaze::ClientState::Status>
    {
    public:
        static const PyroCustomEnumValues<Blaze::ClientState::Status> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::ClientState::Status> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::ClientState::Status>("Blaze::ClientState::Status::Status")
        {
            add("STATUS_NORMAL", (int32_t)Blaze::ClientState::STATUS_NORMAL);
            add("STATUS_SUSPENDED", (int32_t)Blaze::ClientState::STATUS_SUSPENDED);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::ClientState::Mode> : public EnumNameValueCollectionT<Blaze::ClientState::Mode>
    {
    public:
        static const PyroCustomEnumValues<Blaze::ClientState::Mode> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::ClientState::Mode> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::ClientState::Mode>("Blaze::ClientState::Mode::Mode")
        {
            add("MODE_UNKNOWN", (int32_t)Blaze::ClientState::MODE_UNKNOWN);
            add("MODE_MENU", (int32_t)Blaze::ClientState::MODE_MENU);
            add("MODE_SINGLE_PLAYER", (int32_t)Blaze::ClientState::MODE_SINGLE_PLAYER);
            add("MODE_MULTI_PLAYER", (int32_t)Blaze::ClientState::MODE_MULTI_PLAYER);
        }
    };

    template<>
    class PyroCustomEnumValues<Blaze::Authentication::CrossPlatformOptSetting> : public EnumNameValueCollectionT<Blaze::Authentication::CrossPlatformOptSetting>
    {
    public:
        static const PyroCustomEnumValues<Blaze::Authentication::CrossPlatformOptSetting> &getInstance()
        {
            static PyroCustomEnumValues<Blaze::Authentication::CrossPlatformOptSetting> instance;
            return instance;
        }

    private:
        PyroCustomEnumValues()
            : EnumNameValueCollectionT<Blaze::Authentication::CrossPlatformOptSetting>("Blaze::Authentication::CrossPlatformOptSetting")
        {
            add("DEFAULT", (int32_t)Blaze::Authentication::CrossPlatformOptSetting::DEFAULT);
            add("OPTIN", (int32_t)Blaze::Authentication::CrossPlatformOptSetting::OPTIN);
            add("OPTOUT", (int32_t)Blaze::Authentication::CrossPlatformOptSetting::OPTOUT);
        }
    };
}

#endif
