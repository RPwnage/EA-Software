#ifndef PENDING_ACTION_DEFINES_H
#define PENDING_ACTION_DEFINES_H

namespace Origin
{
    namespace Client
    {
        namespace PendingAction
        {
            //action types
            const char StoreOpenLookupId[] = "store/open";
            const char LibraryOpenLookupId[] = "library/open";
            const char GameLaunchLookupId[] = "game/launch";
            const char CurrentTabLookupId[] = "currenttab";
            const char GameDownloadLookupId[] = "game/download";

            //tab types
            const char StartupTabMyGamesId[] = "mygames";
            const char StartupTabStoreId[] = "store";

            //possible query params
            const char ParamRefreshId[] = "refresh";
            const char ParamClientPageId[] = "clientPage";
            const char ParamIdId[] = "id";
            const char ParamAuthCodeId[] = "authCode";
            const char ParamAuthTokenId[] = "authToken";
            const char ParamAuthRedirectUrlId[] = "authRedirectUrl";
            const char ParamPopupWindowId[] = "popupWindow";
            const char ParamWindowModeId[] = "windowMode";
            const char ParamPersonaId[] = "persona";
            const char ParamContextTypeId[] = "contextType";
            const char ParamOfferIdsId[] = "offerIds";
            const char ParamAutoDownloadId[] = "autoDownload";
            const char ParamTitleId[] = "title";
            const char ParamCmdParamsId[] = "cmdParams";
            const char ParamRestartId[] = "restart";
            const char ParamItemSubTypeId[] = "itemSubType";
            const char ParamRtpFile[] = "rtpfile";
            const char ParamForceOnlineId[] = "forceOnline";
            const char ParamRequestorId[] = "requestorId";
            const char ParamRedeemCodeId[] = "redeemCode";
            const char ParamOfferIdId[] = "offerId";
            const char ParamFrontOfQueue[] = "frontOfQueue";
            const char ParamNoPromo[] = "noPromo";

            //unexposed params used internally
            const char ParamInternalJumpId[] = "jump";       //to keep track of whether protocol was initiated from jumplist

            //contextTypes
            const char ContextTypeOfferId[] = "OFFER";
            const char ContextTypeMasterTitleId[] = "MASTERTITLE";

            //requestor ids for reddem
            const char RequestorIdClient[] = "REDEEM_CODE_CLIENT";
            const char RequestorIdITO[] = "REDEEM_CODE_ITO";
            const char RequestorIdRTP[] = "REDEEM_CODE_RTP";
            const char RequestorIdWeb[] = "REDEEM_CODE_WEB";

            //popup window types
            const char PopupWindowFriendChatId[] = "FRIEND_CHAT_WINDOW";
            const char PopupWindowFriendListId[] = "FRIEND_LIST_WINDOW";
            const char PopupWindowAddGameId[] = "ADD_GAME_WINDOW";
            const char PopupWindowRedeemGameId[] = "REDEEM_GAME_WINDOW";
            const char PopupWindowAddFriendId[] = "ADD_FRIEND_WINDOW";
            const char PopupWindowHelpId[] = "HELP_WINDOW";
            const char PopupWindowChangeAvatarId[] = "CHANGE_AVATAR_WINDOW";
            const char PopupWindowAboutId[] = "ABOUT_WINDOW";

            //client window status types
            const char ClientWindowNormalId[] = "NORMAL";
            const char ClientWindowMinimizedToTrayId[] = "MINIMIZED_TO_TRAY";
            const char ClientWindowMinimizedToTaskbarId[] = "MINIMIZED_TO_TASKBAR";
            const char ClientWindowMaximizedId[] = "MAXIMIZED";

            //client page types
            const char ClientPageGameDetailsByOfferId[] = "GAMEDETAILS_BY_OFFER";
            const char ClientPageGameDetailsByContentId[] = "GAMEDETAILS_BY_CONTENT";
            const char ClientPageGameDetailsByUnknownTypeId[] = "GAMEDETAILS_BY_UNKNOWNTYPE";
            const char ClientPageProfileId[] = "PROFILE";
            const char ClientPageAchievementsId[] = "ACHIEVEMENTS";
            const char ClientPageSettingsGeneralId[] = "SETTINGS_GENERAL";
            const char ClientPageSettingsNotificationsId[] = "SETTINGS_NOTIFICATIONS";
            const char ClientPageSettingsOIGId[] = "SETTINGS_OIG";
            const char ClientPageSettingsAdvancedId[] = "SETTINGS_ADVANCED";
            const char ClientPageAccountAndProfileAboutMeId[] = "ACCOUNT_AND_PROFILE_ABOUTME";
            const char ClientPageAccountAndProfilePrivacyId[] = "ACCOUNT_AND_PROFILE_PRIVACY";
            const char ClientPageAccountAndProfileSecurityId[] = "ACCOUNT_AND_PROFILE_SECURITY";
            const char ClientPageAccountAndProfileOrderHistoryId[] = "ACCOUNT_AND_PROFILE_ORDER_HISTORY";
            const char ClientPageAccountAndProfilePaymentId[] = "ACCOUNT_AND_PROFILE_PAYMENT";
            const char ClientPageAccountAndProfileOrderRedeemId[] = "ACCOUNT_AND_PROFILE_ORDER_REDEEM";

            //openautomate
            const char OpenAutomateId[] = "openautomate";

            const char LatestOriginScheme[] = "origin2";

            //telemetry reporting, need to keep it the same as before to preserve telemetry data
            const char TelemetryAddFriend[]     = "addfriend";
            const char TelemetryAddGame[]       = "addgame";
            const char TelemetryFriends []      = "friends";
            const char TelemetryHelp []         = "help";
            const char TelemetryProfile []      = "profile";
            const char TelemetryRedeem []       = "redeem";
            const char TelemetrySettings []     = "settings";

            enum ActionComponents
            {
                NoComponents = 0,
                MainAction = 1 << 1,
                PopupWindow = 1 << 2,
                WindowMode = 1 << 4,
                AllComponents = MainAction | PopupWindow | WindowMode
            };
        }
    }
}

#endif