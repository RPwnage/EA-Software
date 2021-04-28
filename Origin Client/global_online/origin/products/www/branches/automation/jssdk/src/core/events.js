/*jshint unused: false */
/*jshint strict: false */
define([
    'core/utils'
], function(utils) {

    /**
     * The Origin JSSDK notfies integrators of the following events. Use <i>Origin.events.on(Origin.events.eventName, callback)</i> to register a listener. Use <i>Origin.events.off(Origin.events.eventName, callback)</i> to unregister a listener.
     * @module module:events
     * @memberof module:Origin
     */

    var publicEventEnums = {
        /**
         * The Origin JSSDK authenticated successfully and we have an access token and user pid. No objects are passed with this event.
         * @event AUTH_USERPIDRETRIEVED
         */
        AUTH_USERPIDRETRIEVED: 'authUserPidRetrieved',
        /**
         * The Origin JSSDK authenticated successfully and we have an access token and user pid. No objects are passed with this event.
         * @event AUTH_SUCCESS_LOGIN
         */
        AUTH_SUCCESS_LOGIN: 'authSuccessLogin',
        /**
         * fired when authentication succeeds after coming back online from offline mode. No objects are passed with this event.
         * @event AUTH_SUCCESS_POST_OFFLINE
         */
        AUTH_SUCCESS_POST_OFFLINE: 'authSuccessPostOffline',
        /**
         * fired when authentication succeeds after loginType APP_RETRY_LOGIN is passed in as parameter to login
         * @event AUTH_SUCCESS_RETRY
         */
        AUTH_SUCCESS_RETRY: 'authSuccessRetry',
        /**
         * The Origin JSSDK cannot authenticate and the integrator must ask the user to login. No objects are passed with this event.
         * @event AUTH_FAILED_CREDENTIAL
         */
        AUTH_FAILED_CREDENTIAL: 'authFailedCredential',
        /**
         * fired when authentication fails after coming back online from offline mode. No objects are passed with this event.
         * @event AUTH_FAILED_POST_OFFLINE
         */
        AUTH_FAILED_POST_OFFLINE: 'authFailedPostOffline',
        /**
         * fired when authentication fails after loginType APP_RETRY_LOGIN is passed in as parameter to login
         * @event AUTH_FAILED_RETRY
         */
        AUTH_FAILED_RETRY: 'authFailedRetry',
        /**
         * The Origin JSSDK has logged out. No objects are passed with this event. No objects are passed with this event.
         * @event AUTH_LOGGEDOUT
         */
        AUTH_LOGGEDOUT: 'authLoggedOut',
        /**
         * The user has successfully connected to the chat server. No objects are passed with this event.
         * @event XMPP_CONNECTED
         */
        XMPP_CONNECTED: 'xmppConnected',
        /**
         * The user has been disconnected to the chat server. No objects are passed with this event.
         * @event XMPP_DISCONNECTED
         * @memberof Origin.events
         */
        XMPP_DISCONNECTED: 'xmppDisconnected',
        /**
         * The user has logged in with the same resource somewhere else and will be dioscnnected. No objects are passed with this event.
         * @event XMPP_USERCONFLICT
         */
        XMPP_USERCONFLICT: 'xmppUserConflict',
        /**
         * This object is passed through the {@link Origin.events.event:xmppIncomingMsg Origin.events.xmppIncomingMsg} event
         * @typedef xmppMsgObject
         * @type {object}
         * @property {string} jid The user's jabber id.
         * @property {string} msgBody The chat message.
         * @property {string} chatState possible chat states for users
         *   <ul>
            <li>'ACTIVE' - User is actively participating in the chat session.
            <li>'INACTIVE' - User has not been actively participating in the chat session.
            <li>'GONE' - User has effectively ended their participation in the chat session
            <li>'PAUSED' - User had been composing but now has stopped.
            <li>'COMPOSING' - User is composing a message.
            </ul>
         */
        /**
         * This event notifies users of any incoming chat messages. A {@link xmppMsgObject} is passed with the event.
         * @event XMPP_INCOMINGMSG
         *
         */
        XMPP_INCOMINGMSG: 'xmppIncomingMsg',

        //currently not passing this parameter back in xmppPresenceObject because this data doesn't exist in the C++ client presence info
        /*
         @property {string} presenceType States to describe the current user's presence.
            <ul>
            <li>'AVAILABLE' - Signals that the user is online and available for communication.
            <li>'UNAVAILABLE' - Signals that the entity is no longer available for communication.
            <li>'SUBSCRIBE' - The sender wishes to subscribe to the recipient's presence.
            <li>'SUBSCRIBED' - The sender has allowed the recipient to receive their presence.
           <li>'UNSUBSCRIBE' - The sender is unsubscribing from another entity's presence.
            <li>'UNSUBSCRIBED' - The subscription request has been denied or a previously-granted subscription has been cancelled.
            <li>'PROBE' - A request for an entity's current presence; SHOULD be generated only by a server on behalf of a user.
            <li>'ERROR' - An error has occurred regarding processing or delivery of a previously-sent presence stanza.
            </ul>
        */

        /**
         * This object is passed through the {@link Origin.events.event:xmppPresenceChanged Origin.events.xmppPresenceChanged} event
         * @typedef xmppPresenceObject
         * @type {object}
         * @property {string} jid The user's jabber id.
         * @property {string} show Specifies the particular availability status of an entity or specific resource.
            <ul>
            <li>'ONLINE' = The entity is online and available.
            <li>'AWAY' - The entity or resource is temporarily away.
            <li>'CHAT' - The entity or resource is actively interested in chatting.
            <li>'DND' - The entity or resource is busy (dnd = "Do Not Disturb").
            <li>'XA' - The entity or resource is away for an extended period (xa = "eXtended Away").
            </ul>
         */
        /**
         * This event notifies users of any presence changes of users you're subscribed to. A {@link xmppPresenceObject} is passed with the event.
         * @event XMPP_PRESENCECHANGED
         *
         */
        XMPP_PRESENCECHANGED: 'xmppPresenceChanged',

        /**
         * This object is passed through the {@link Origin.events.event:xmppPresenceVisibilityChanged Origin.events.xmppPresenceVisibilityChanged} event
         * @typedef xmppPresenceObject
         * @type {object}
         * @property {string} jid The user's jabber id.
         * @property {boolean} show Specifies the particular visibility of the user.
         */
        /**
         * This event notifies users of any presence visibility changes of users you're subscribed to. A {@link xmppPresenceObject} is passed with the event.
         * @event XMPP_PRESENCEVISIBILITYCHANGED
         *
         */
        XMPP_PRESENCEVISIBILITYCHANGED: 'xmppPresenceVisibilityChanged',

        /**
         * This object is passed through the {@link Origin.events.event:xmppRosterChanged Origin.events.xmppRosterChanged} event
         * @typedef xmppRosterChangeObject
         * @type {object}
         * @property {string} jid The user's jabber id.
         * @property {string} substate The subscription state of the user.
            <ul>
            <li>'NONE' = The user does not have a subscription to the contact's presence, and the contact does not have a subscription to the user's presenc
            <li>'TO' - The user has a subscription to the contact's presence, but the contact does not have a subscription to the user's presence.
            <li>'FROM' - The contact has a subscription to the user's presence, but the user does not have a subscription to the contact's presence.
            <li>'BOTH' - The user and the contact have subscriptions to each other's presence (also called a "mutual subscription").
            <li>'REMOVE' - The contact has removed the user.
            </ul>
         */
        /**
         * This event notifies users the their social roster has changed. A {@link xmppRosterChangeObject} is passed with the event.
         * @event XMPP_ROSTERCHANGED
         *
         */
        XMPP_ROSTERCHANGED: 'xmppRosterChanged',

        /**
         * This object is a part of the {@link clientGamesObject}
         * @typedef progressObject
         * @type {object}
         * @property {boolean} active true if the game is running a download/update/repair/install
         * @property {string} phase
         * @property {string} phaseDisplay The display text for the progress state.
         * @property {number} The progress range from 0 to 1.
         * @property {string} progressState additional progressInfo
         */
        /**
         * This object is a part of the {@link clientGamesObject}
         * @typedef dialogInfoObject
         * @type {object}
         * @property {boolean} showCancel Integrator should show a cancel dialog
         * @property {boolean} showDownloadInfo Integrator should show a download info dialog
         * @property {boolean} showEula Integrator should show a eula dialog
         */
        /**
         * This object is passed through the {@link Origin.events.event:clientGamesChanged Origin.events.clientGamesChanged} event
         * @typedef clientGamesObject
         * @type {object}
         * @property {bool} cancellable Can the game download be cancelled.
         * @property {dialogInfoObject} dialogInfo Info related to download dialog.
         * @property {bool} downloadable Can the game be downloaded.
         * @property {bool} downloading Is the game downloading.
         * @property {bool} installable Can the game be installed.
         * @property {bool} installed Is the game installed.
         * @property {bool} installing Is the game installing.
         * @property {bool} pausable Can the game downloadbe paused.
         * @property {bool} playable Is the game playable.
         * @property {bool} playing Is the user playing.
         * @property {string} productId The product id of the game.
         * @property {progressObject} progressInfo Progress related info, only valid if the active flag is true.
         * @property {number} queueIndex The position in the download queue.
         * @property {bool} queueSkippingEnabled Can the queue be skipped.
         * @property {bool} repairSupported Is repairing supported for this game.
         * @property {bool} repairing Is the game repairing.
         * @property {bool} resumable Can the game be resumed.
         * @property {bool} updateSupported Is updating supported for this game.
         * @property {bool} updating Is the game updating
         */
        /**
         * This event is fired when we receive updated game status info. It will also fire when you call {@link Origin.client.games.requestGamesStatus}. A {@link clientGamesObject} is passed with the event.
         * @event CLIENT_GAMES_CHANGED
         *
         */
        CLIENT_GAMES_CHANGED: 'clientGamesChanged',
        /**
         * This event is fired when we receive updated game status info. It will also fire when you call {@link Origin.client.games.requestGamesStatus}. A {@link clientGamesObject} is passed with the event.
         * @event CLIENT_GAMES_CHANGED
         *
         */
        CLIENT_GAMES_CLOUD_USAGE_CHANGED: 'clientGamesCloudUsageChanged',
        /**
         * This event is fired when a dialog should be show. A dynamic json object is passed with this event
         * @event CLIENT_DIALOGOPEN
         */
        CLIENT_DIALOGOPEN: 'clientGamesDialogOpen',
        /**
         * This event is fired when a dialog should be closed. A dynamic json object is passed with this event
         * @event CLIENT_DIALOGCLOSED
         */
        CLIENT_DIALOGCLOSED: 'clientGamesDialogClosed',
        /**
         * This event is fired when a dialog should be updated. A dynamic json object is passed with this event
         * @event CLIENT_DIALOGCHANGED
         */
        CLIENT_DIALOGCHANGED: 'clientGamesDialogChanged',
        /**
         * This event is fired when the user clicks on the dock icon.
         * @event CLIENT_DOCK_ICONCLICKED
         */
        CLIENT_DESKTOP_DOCKICONCLICKED: 'clientDockIconClicked',
        /**
         * This event is fired when we receive a SDK signal from the game to report a user
         * @event CLIENT_GAME_REQUESTREPORTUSER
         */
        CLIENT_GAME_REQUESTREPORTUSER: 'clientGameRequestReportUser',
        /**
         * This event is fired when we receive a SDK signal from the game to invite friends
         * @event CLIENT_GAME_INVITEFRIENDSTOGAME
         */
        CLIENT_GAME_INVITEFRIENDSTOGAME: 'clientGameInviteFriendsToGame',
        /**
         * This event is fired when we receive a SDK signal from the game to start a conversation
         * @event CLIENT_GAME_STARTCONVERSATION
         */
        CLIENT_GAME_STARTCONVERSATION: 'clientGameStartConversation',
        /**
         * This event is fired when we receive a signal that the initial base game update has been completed
         * @event CLIENT_GAMES_BASEGAMESUPDATED
         */
        CLIENT_GAMES_BASEGAMESUPDATED: 'clientGamesBaseGamesUpdated',
        /**
         * This event is fired when we receive a list of games that have either been added or removed. A list of added offerIds and a list of removed offerIds are passed with the event.
         * @event CLIENT_GAMES_LISTCHANGED
         */
        CLIENT_GAMES_LISTCHANGED: 'clientGamesListChanged',
        /**
         * This event is fired when we receive an updated progress status info.
         * @event CLIENT_GAMES_PROGRESSCHANGED
         */
        CLIENT_GAMES_PROGRESSCHANGED: 'clientGamesProgressChanged',
        /**
         * This event is fired when we receive an signal telling us that a game's operation failed (download, update, repair, etc)
         * @event CLIENT_GAMES_OPERATIONFAILED
         */
        CLIENT_GAMES_OPERATIONFAILED: 'clientGamesOperationFailed',
        /**
         * This event is fired when the client has updated play time for a game.
         * @event CLIENT_GAMES_PLAYTIMECHANGED
         */
        CLIENT_GAMES_PLAYTIMECHANGED: 'clientGamesPlayTimeChanged',
        /**
         * This event is fired when we receive a change in downloadqueue with a list of queue info for each entitlement
         * @event CLIENT_GAMES_DOWNLOADQUEUECHANGED
         */
        CLIENT_GAMES_DOWNLOADQUEUECHANGED: 'clientGamesDownloadQueueChanged',
        /**
         * This event is fired when a NOG has been updated
         * @event CLIENT_GAMES_NOGUPDATED
         */
        CLIENT_GAMES_NOGUPDATED: 'clientGamesNogUpdated',
        /**
         * This event is fired when the Origin client connection state has changed. A boolean is passed with the event. True means the client went online. False means the client went offline. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_ONLINESTATECHANGED
         */
        CLIENT_ONLINESTATECHANGED: 'clientOnlineStateChanged',
        /**
         * This event is fired when the user clicks the titlebar offline mode button [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_CLICKEDOFFLINEMODEBUTTON
         */
        CLIENT_CLICKEDOFFLINEMODEBUTTON: 'clientClickedOfflineModeButton',
        /**
         * This event is fired when the chat roster is loaded initially. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_ROSTERLOADED
         */
        CLIENT_SOCIAL_ROSTERLOADED: 'clientSocialRosterLoaded',
        /**
         * This event is fired when the presence has changed for the user or friends. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_PRESENCECHANGED
         */
        CLIENT_SOCIAL_PRESENCECHANGED: 'clientSocialPresenceChanged',
        /**
         * This event is fired when the block list has changed. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_BLOCKLISTCHANGED
         */
        CLIENT_SOCIAL_BLOCKLISTCHANGED: 'clientSocialBlockListChanged',
        /**
         * This event is fired when the social connection has changed. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_CONNECTIONCHANGED
         */
        CLIENT_SOCIAL_CONNECTIONCHANGED: 'clientSocialConnectionChanged',
        /**
         * This event is fired when a new message is received from client. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_MESSAGERECEIVED
         */
        CLIENT_SOCIAL_MESSAGERECEIVED: 'clientSocialMessageReceived',
        /**
         * This event is fired when the chat state has changes from client. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_CHATSTATERECEIVED
         */
        CLIENT_SOCIAL_CHATSTATERECEIVED: 'clientSocialChatStateReceived',
        /**
         * This event is fired when a friend has been added or removed. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_ROSTERCHANGED
         */
        CLIENT_SOCIAL_ROSTERCHANGED: 'clientSocialRosterChanged',
        /**
         * This event is fired when a friend invites you to a game. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_GAMEINVITERECEIVED
         */
        CLIENT_SOCIAL_GAMEINVITERECEIVED: 'clientSocialGameInviteReceived',
        /**
         * This event is fired when the chat window associated with a friend is to be shown)
         * @event CLIENT_SOCIAL_SHOWCHATWINDOWFORFRIEND
         */
        CLIENT_SOCIAL_SHOWCHATWINDOWFORFRIEND: 'showChatWindowForFriend',
        /**
         * This event is fired when the friends list pop out needs to be brought into focus. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_FOCUSONFRIENDSLIST
         */
        CLIENT_SOCIAL_FOCUSONFRIENDSLIST: 'focusOnFriendsList',
        /**
         * This event is fired when the chat window pop out needs to be brought into focus. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SOCIAL_FOCUSONACTIVECHATWINDOW
         */
        CLIENT_SOCIAL_FOCUSONACTIVECHATWINDOW: 'focusOnActiveChatWindow',
        /**
         * This event is fired when there is a SID update [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_SIDRENEWAL
         */
        CLIENT_SIDRENEWAL: 'clientSidRenewal',
        /**
         * An entitlement was added to the content operation queue.
         * @event CLIENT_OPERATIONQUEUE_ENQUEUED
         * @property {Object} offer id of entitlement being added to queue
         */
        CLIENT_OPERATIONQUEUE_ENQUEUED: 'clientOperationQueueEnqueued',
        /**
         * An entitlement was removed to the content operation queue.
         * @event CLIENT_OPERATIONQUEUE_REMOVED
         * @property {String} offer id of entitlement being removed
         * @property {Boolean} true/false if the children entitlements should be removed
         * @property {Boolean} true/false if we enqueue the next item
         */
        CLIENT_OPERATIONQUEUE_REMOVED: 'clientOperationQueueRemoved',
        /**
         * An entitlement was added to the content operation queue completed list.
         * @event CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE
         * @property {Object} offer id of entitlement that was added to the completed list
         */
        CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE: 'clientOperationQueueAddedToComplete',
        /**
         * The content operation queue completed list was cleared.
         * @event CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED
         */
        CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED: 'clientOperationQueueCompleteListCleared',
        /**
         * Has the head item gone into or out of an install state
         * @event CLIENT_OPERATIONQUEUE_HEADBUSY
         * @property {Boolean} True/false if the head of the queue went in or out of busy
         */
        CLIENT_OPERATIONQUEUE_HEADBUSY: 'clientOperationQueueHeadBusy',
        /**
         * The head of the operation queue has changed.
         * @event CLIENT_OPERATIONQUEUE_HEADCHANGED
         * @property {Object} Offer id of the new head entitlement
         * @property {Object} Offer id of the old head entitlement
         */
        CLIENT_OPERATIONQUEUE_HEADCHANGED: 'clientOperationQueueHeadChanged',
        /**
         * This event is fired when the settings have been updated
         * @event CLIENT_SETTINGS_UPDATESETTINGS
         */
        CLIENT_SETTINGS_UPDATESETTINGS: 'clientSettingsUpdateSettings',
        /**
         * This event is fired when returning from a settings dialog
         * @event CLIENT_SETTINGS_RETURN_FROM_DIALOG
         */
        CLIENT_SETTINGS_RETURN_FROM_DIALOG: 'clientSettingsReturnFromDialog',
        /**
         * This event is fired when there is an error setting a setting (e.g. hotkey conflict)
         * @event CLIENT_SETTINGS_ERROR
         */
        CLIENT_SETTINGS_ERROR: 'clientSettingsError',
        /**
         * This event is fired when a voice device like a headset is added [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_DEVICEADDED
         */
        CLIENT_VOICE_DEVICEADDED: 'clientVoiceDeviceAdded',
        /**
         * This event is fired when a voice device like a headset is removed [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_DEVICEREMOVED
         */
        CLIENT_VOICE_DEVICEREMOVED: 'clientVoiceDeviceRemoved',
        /**
         * This event is fired when the default voice device changed [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_DEFAULTDEVICECHANGED
         */
        CLIENT_VOICE_DEFAULTDEVICECHANGED: 'clientVoiceDefaultDeviceChanged',
        /**
         * This event is fired when a voice device like a headset has changed  [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_DEVICECHANGED
         */
        CLIENT_VOICE_DEVICECHANGED: 'clientVoiceDeviceChanged',
        /**
         * This event is fired when there is a voice level change  [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_VOICELEVEL
         */
        CLIENT_VOICE_VOICELEVEL: 'clientVoiceVoiceLevel',
        /**
         * This event is fired when the voice device is under threshold [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_UNDERTHRESHOLD
         */
        CLIENT_VOICE_UNDERTHRESHOLD: 'clientVoiceUnderthreshold',
        /**
         * This event is fired when the voice device is over threshold [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_OVERTHRESHOLD
         */
        CLIENT_VOICE_OVERTHRESHOLD: 'clientVoiceOverthreshold',
        /**
         * This event is fired when we have made a voice connection [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_VOICECONNECTED
         */
        CLIENT_VOICE_VOICECONNECTED: 'clientVoiceConnected',
        /**
         * This event is fired when we have stopped a voice connection [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_VOICEDISCONNECTED
         */
        CLIENT_VOICE_VOICEDISCONNECTED: 'clientVoiceDisconnected',
        /**
         * This event is fired when we the test microphone is enabled [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_ENABLETESTMICROPHONE
         */
        CLIENT_VOICE_ENABLETESTMICROPHONE: 'clientEnableTestMicrophone',
        /**
         * This event is fired when we the test microphone is disabled [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_DISABLETESTMICROPHONE
         */
        CLIENT_VOICE_DISABLETESTMICROPHONE: 'clientDisableTestMicrophone',
        /**
         * This event is fired when we should clear the level indicator [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_CLEARLEVELINDICATOR
         */
        CLIENT_VOICE_CLEARLEVELINDICATOR: 'clientVoiceClearLevelIndicator',
        /**
         * This event is fired when a voice call event occurs [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_VOICE_VOICECALLEVENT
         */
        CLIENT_VOICE_VOICECALLEVENT: 'clientVoiceCallEvent',
        /**
         * This event is fired when the enbedded client causes navigation to the MyGames tab [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_NAV_MYGAMES
         */
        CLIENT_NAV_ROUTECHANGE: 'clientNavRouteChange',
        /**
         * This event is fired when the enbedded client causes navigation to the Store tab by Product Id [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_NAV_STOREBYPRODUCTID
         */
        CLIENT_NAV_STOREBYPRODUCTID: 'navigateToStoreByProductId',
        /**
         * This event is fired when the enbedded client causes navigation to the Store tab by Master Title [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_NAV_STOREBYMASTERTITLE
         */
        CLIENT_NAV_STOREBYMASTERTITLEID: 'navigateToStoreByMasterTitleId',
        /**
         * This event is fired when the enbedded client causes the find friends modal to open [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_NAV_OPENMODAL_FINDFRIENDS
         */
        CLIENT_NAV_OPENMODAL_FINDFRIENDS: 'clientOpenModalFindFriends',
        /**
         * This event is fired when the enbedded client causes the download queue flyout to open [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_NAV_OPEN_DOWNLOADQUEUE
         */
        CLIENT_NAV_OPEN_DOWNLOADQUEUE: 'clientOpenDownloadQueue',
        /**
         * This event is fired when the enbedded client causes the search box to focus [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_NAV_FOCUSONSEARCH
         */
        CLIENT_NAV_FOCUSONSEARCH: 'focusOnSearch',
        /**
         * The client is requesting a pending update stripe.
         * @event CLIENT_NAV_SHOW_PENDING_UPDATE_STRIPE
         */
        CLIENT_NAV_SHOW_PENDING_UPDATE_STRIPE: 'showPendingUpdateSitestripe',
        /**
         * The client is requesting a pending update stripe with countdown.
         * @event CLIENT_NAV_SHOW_PENDING_UPDATE_COUNTDOWN_STRIPE
         */
        CLIENT_NAV_SHOW_PENDING_UPDATE_COUNTDOWN_STRIPE: 'showPendingUpdateCountdownSitestripe',
        /**
         * The client is requesting a kick offline stripe.
         * @event CLIENT_NAV_SHOW_PENDING_UPDATE_KICKED_OFFLINE_STRIPE
         */
        CLIENT_NAV_SHOW_PENDING_UPDATE_KICKED_OFFLINE_STRIPE: 'showPendingUpdateKickedOfflineSitestripe',
        /**
         * The client is requesting to open the game details page
         * @event CLIENT_NAV_OPEN_GAME_DETAILS
         */
        CLIENT_NAV_OPEN_GAME_DETAILS: 'openGameDetails',
        /**
         * The client is requesting to open subscription checkout
         * @event CLIENT_NAV_RENEW_SUBSCRIPTION
         */
        CLIENT_NAV_RENEW_SUBSCRIPTION: 'renewSubscription',
        /**
         * The client has been either minimized or restored. A boolean is passed in to indicate if the client just became visible.
         * @event CLIENT_VISIBILITY_CHANGED
         */
        CLIENT_VISIBILITY_CHANGED: 'clientVisibilityChanged',

        /**
         * The client popout has closed
         * @event CLIENT_POP_OUT_CLOSED
         */
        CLIENT_POP_OUT_CLOSED: 'clientPopoutClosed',
        /**
         * This event is fired when the settings data is ready.
         * @event SETTINGS_DATAREADY
         */
        SETTINGS_DATAREADY: 'settingsDataReady',
        /**
         * This event is fired when the settings data failed to retrieve.
         * @event SETTINGS_DATAFAILURE
         */
        SETTINGS_DATAFAILURE: 'settingsDataFailure',
        /**
         * This event is fired when a voice call event occurs.
         * @event VOICE_CALL
         * @memberof Origin.events
         */
        VOICE_CALL: 'voiceCall',
        /**
         * This event is fired when a voice level change occurs.
         * @event VOICE_LEVEL
         * @memberof Origin.events
         */
        VOICE_LEVEL: 'voiceLevel',
        /**
         * This event is fired when a voice device is added.
         * @event VOICE_DEVICE_ADDED
         * @memberof Origin.events
         */
        VOICE_DEVICE_ADDED: 'voiceDeviceAdded',
        /**
         * This event is fired when a voice device is removed.
         * @event VOICE_DEVICE_REMOVED
         * @memberof Origin.events
         */
        VOICE_DEVICE_REMOVED: 'voiceDeviceRemoved',

        /**
         * This event is fired when a default audio device was changed.
         * @event VOICE_DEFAULT_DEVICE_CHANGED
         * @memberof Origin.events
         */
        VOICE_DEFAULT_DEVICE_CHANGED: 'voiceDefaultDeviceChanged',
        /**
         * This event is fired when a voice device is changed. [FOR WINDOWS XP]
         * @event VOICE_DEVICE_CHANGED
         * @memberof Origin.events
         */
        VOICE_DEVICE_CHANGED: 'voiceDeviceChanged',
        /**
         * This event is fired when the voice level is under the activation threshold.
         * @event VOICE_UNDER_THRESHOLD
         * @memberof Origin.events
         */
        VOICE_UNDER_THRESHOLD: 'voiceUnderThreshold',

        /**
         * This event is fired when the voice level is over the activation threshold.
         * @event VOICE_OVER_THRESHOLD
         * @memberof Origin.events
         */
        VOICE_OVER_THRESHOLD: 'voiceOverThreshold',
        /**
         * This event is fired when a voice channel is connected.
         * @event VOICE_CONNECTED
         * @memberof Origin.events
         */
        VOICE_CONNECTED: 'voiceConnected',
        /**
         * This event is fired when a voice channel is disconnected.
         * @event VOICE_DISCONNECTED
         * @memberof Origin.events
         */
        VOICE_DISCONNECTED: 'voiceDisconnected',

        /**
         * This event is fired when the client wants to enable the 'test mic' action link.
         * @event VOICE_ENABLE_TEST_MICROPHONE
         * @memberof Origin.events
         */
        VOICE_ENABLE_TEST_MICROPHONE: 'voiceEnableTestMicrophone',
        /**
         * This event is fired when the client wants to disable the 'test mic' action link.
         * @event VOICE_DEVICE_ADDED
         * @memberof Origin.events
         */
        VOICE_DISABLE_TEST_MICROPHONE: 'voiceDisableTestMicrophone',
        /**
         * This event is fired when the client wants to clear the 'test mic' level indicator.
         * @event VOICE_CLEAR_LEVEL_INDICATOR
         * @memberof Origin.events
         */
        VOICE_CLEAR_LEVEL_INDICATOR: 'voiceClearLevelIndicator',

        /**
         * This event is fired when the locale value changes
         * @event LOCALE_CHANGED
         */
        LOCALE_CHANGED: 'localeChanged',

        /**
         * This event is fired when the language code value changes
         * @event LANGUAGE_CODE_CHANGED
         */
        LANGUAGE_CODE_CHANGED: 'languageCodeChanged',

        /**
         * This event is fired when the country code value changes
         * @event COUNTRY_CODE_CHANGED
         */
        COUNTRY_CODE_CHANGED: 'countryCodeChanged',

        /**
         * This event is fired when the three letter country code value changes
         * @event THREE_LETTER_COUNTRY_CODE_CHANGED
         */
        THREE_LETTER_COUNTRY_CODE_CHANGED: 'threeLetterCountryCodeChanged',

        /**
         * This event is fired when the currency code changes
         * @event CURRENCY_CODE_CHANGED
         */
        CURRENCY_CODE_CHANGED: 'currencyCodeChanged',

        /**
         * This event notifies users of changes to the block list.
         * @event XMPP_BLOCKLISTCHANGED
         *
         */
        XMPP_BLOCKLISTCHANGED: 'xmppBlockListChanged',
        /**
         * This event is fired when the user receives a game invite from a friend
         * @event XMPP_GAMEINVITERECEIVED
         *
         */
        XMPP_GAMEINVITERECEIVED: 'xmppGameInviteReceived',
        /**
         * This event notifies users of changes to the achievements.
         * @event DIRTYBITS_ACHIEVEMENTS
         *
         */
        DIRTYBITS_ACHIEVEMENTS: 'dirtyBitAchievements',
        /**
         * This event notifies users of changes to social groups.
         * @event DIRTYBITS_GROUP
         *
         */
        DIRTYBITS_GROUP: 'dirtyBitsGroups',
        /**
         * This event notifies users of changes to email.
         * @event DIRTYBITS_EMAIL
         *
         */
        DIRTYBITS_EMAIL: 'dirtyBitsEmail',
        /**
         * This event notifies users of changes to password.
         * @event DIRTYBITS_PASSWORD
         *
         */
        DIRTYBITS_PASSWORD: 'dirtyBitsPassword',
        /**
         * This event notifies users of changes to origin id.
         * @event DIRTYBITS_ORIGINID
         *
         */
        DIRTYBITS_ORIGINID: 'dirtyBitsOriginId',
        /**
         * This event notifies users of changes to game lib.
         * @event DIRTYBITS_GAMELIB
         *
         */
        DIRTYBITS_GAMELIB: 'dirtyBitsGameLib',
        /**
         * This event notifies users of changes to users privacy settings.
         * @event DIRTYBITS_PRIVACY
         *
         */
        DIRTYBITS_PRIVACY: 'dirtyBitsPrivacy',
        /**
         * This event notifies users of changes to avatar.
         * @event DIRTYBITS_AVATAR
         *
         */
        DIRTYBITS_AVATAR: 'dirtyBitsAvatar',
        /**
         * This event notifies users of changes to a users entitlement.
         * @event DIRTYBITS_ENTITLEMENT
         *
         */
        DIRTYBITS_ENTITLEMENT: 'dirtyBitsEntitlement',
        /**
         * This event notifies users of changes to catalog.
         * @event DIRTYBITS_CATALOG
         *
         */
        DIRTYBITS_CATALOG: 'dirtyBitsCatalog',
        /**
         * This event notifies users of changes to a users subscription.
         * @event DIRTYBITS_SUBSCRIPTION
         *
         */
        DIRTYBITS_SUBSCRIPTION: 'dirtyBitsSubscription',
        /**
         * The client is requesting to redraw video player
         * @event CLIENT_NAV_REDRAW_VIDEO_PLAYER
         */
        CLIENT_NAV_REDRAW_VIDEO_PLAYER: 'redrawVideoPlayer'

    };

    // add private event enums here
    /** @namespace
     * @memberof privObjs
     * @alias events
     * @private
     */
    var privateEventEnums = {
        priv: {
            /**
             * @event REMOTE_CLIENT_SETUP
             * @memberof publicObj.events.priv
             */
            REMOTE_CLIENT_SETUP: 'remoteClientSetup',
            /**
             * @event REMOTE_CLIENT_AVAILABLE
             * @memberof publicObj.events.priv
             */
            REMOTE_CLIENT_AVAILABLE: 'remoteClientAvailable',
            /**
             * @event REMOTE_STATUS_SENDACTION
             * @memberof publicObj.events.priv
             */
            REMOTE_STATUS_SENDACTION: 'remoteStatusSendAction',
            /**
             * @event REMOTE_CONFIRMATION_FROM_CLIENT
             * @memberof publicObj.events.priv
             */
            REMOTE_CONFIRMATION_FROM_CLIENT: 'remoteConfirmationFromClient',
            /**
             * @event REMOTE_STATUS_UPDATE_FROM_CLIENT
             * @memberof publicObj.events.priv
             */
            REMOTE_STATUS_UPDATE_FROM_CLIENT: 'remoteStatusUpdateFromClient',
            /**
             * @event REMOTE_SEND_CONFIRMATION_TO_CLIENT
             * @memberof publicObj.events.priv
             */
            REMOTE_SEND_CONFIRMATION_TO_CLIENT: 'remoteSendConfirmationToClient',
            /**
             * @event REMOTE_STATUS_INIT
             * @memberof publicObj.events.priv
             */
            REMOTE_STATUS_INIT: 'remoteStatusInit',
            /**
             * Alternate way of triggering login, in case login() is inaccessible due to circular dependency
             * @event AUTH_TRIGGERLOGIN
             * @memberof publicObj.events.priv
             *
             */
            AUTH_TRIGGERLOGIN: 'authTriggerLogin',
            /**
             * fired when login as a result of invalid auth (triggered when 401/403 response received)  succeeds
             * @event AUTH_SUCCESS_POST_AUTHINVALID
             * @memberof publicObj.events.priv
             */
            AUTH_SUCCESS_POST_AUTHINVALID: 'authSuccessPostAuthInvalid',
            /**
             * fired when login as a result of invalid auth (triggered when 401/403 response received)  fails
             * @event AUTH_FAILED_POST_AUTHINVALID
             * @memberof publicObj.events.priv
             */
            AUTH_FAILED_POST_AUTHINVALID: 'authFailedPostAuthInvalid'

        }
    };

    /**
     *
     * Connect a function to one of the Origin JSSDK events
     * @method on
     * @memberof module:Origin.module:events
     * @param {OriginEvent} event one of the event enums described below
     * @param {function} callback the function to be triggered when the event is fired
     * @param {object=} context - the context (what this refers to)
     */

    /**
     *
     * Connect a function to one of the Origin JSSDK events and disconnected after the callback is triggered
     * @method once
     * @memberof module:Origin.module:events
     * @param {OriginEvent} event one of the event enums described below
     * @param {function} callback the function to be triggered when the event is fired
     * @param {object=} context - the context (what this refers to)
     */

    /**
     *
     * Disconnect a function from one of the Origin JSSDK events
     * @method off
     * @memberof module:Origin.module:events
     * @param {OriginEvent} event one of the event enums described below
     * @param {function} callback the function to be disconnected
     * @param {object=} context - the context (what this refers to)
     */
    //for events that are meant to be used by integrators
    var events = new utils.Communicator();

    utils.mix(events, publicEventEnums);
    utils.mix(events, privateEventEnums);

    return events;
});
