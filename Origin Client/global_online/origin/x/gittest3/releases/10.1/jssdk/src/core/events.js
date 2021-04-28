/*jshint unused: false */
/*jshint strict: false */
define([
    'core/utils'
], function (utils) {

    /**
     * The Origin JSSDK notfies integrators of the following events. Use <i>Origin.events.on(Origin.events.eventName, callback)</i> to register a listener. Use <i>Origin.events.off(Origin.events.eventName, callback)</i> to unregister a listener.
     * @namespace
     * @memberof Origin
     * @alias events
     */
    var publicEventEnums = {
        /**
         * The Origin JSSDK authenticated successfully and we have an access token and user pid. No objects are passed with this event.
         * @event AUTH_USERPIDRETRIEVED
         * @memberof Origin.events
         *
         */
        AUTH_USERPIDRETRIEVED: 'authUserPidRetrieved',
        /**
         * The Origin JSSDK authenticated successfully and the integrator call retry calls that require authentication. No objects are passed with this event.
         * @event AUTH_SUCCESSRETRY
         * @memberof Origin.events
         *
         */
        AUTH_SUCCESSRETRY: 'authSuccessRetry',
        /**
         * The Origin JSSDK cannot authenticate and the integrator must ask the user to login. No objects are passed with this event.
         * @event AUTH_FAILEDCREDENTIAL
         * @memberof Origin.events
         */
        AUTH_FAILEDCREDENTIAL: 'authFailedCredential',
        /**
         * The Origin JSSDK has logged out. No objects are passed with this event. No objects are passed with this event.
         * @event AUTH_LOGGEDOUT
         * @memberof Origin.events
         */
        AUTH_LOGGEDOUT: 'authLoggedOut',
        /**
         * The user has successfully connected to the chat server. No objects are passed with this event.
         * @event XMPP_CONNECTED
         * @memberof Origin.events
         */
        XMPP_CONNECTED: 'xmppConnected',
        /**
         * The user has logged in with the same resource somewhere else and will be dioscnnected. No objects are passed with this event.
         * @event XMPP_USERCONFLICT
         * @memberof Origin.events
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
         * @memberof Origin.events
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
         * @memberof Origin.events
         */
        XMPP_PRESENCECHANGED: 'xmppPresenceChanged',
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
         * @memberof Origin.events
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
         * This event is fired when we receive updated game status info. It will also fire when you call {@link Origin.client.games.requestStatus}. A {@link clientGamesObject} is passed with the event.
         * @event CLIENT_GAMES_CHANGED
         * @memberof Origin.events
         */
        CLIENT_GAMES_CHANGED: 'clientGamesChanged',
        /**
         * This event is fired when a dialog should be show. A dynamic json object is passed with this event
         * @event CLIENT_GAMES_DIALOGOPEN
         * @memberof Origin.events
         */
        CLIENT_DIALOGOPEN: 'clientGamesDialogOpen',
        /**
         * This event is fired when a dialog should be closed. A dynamic json object is passed with this event
         * @event CLIENT_GAMES_DIALOGCLOSED
         * @memberof Origin.events
         */
        CLIENT_DIALOGCLOSED: 'clientGamesDialogClosed',
        /**
         * This event is fired when a dialog should be updated. A dynamic json object is passed with this event
         * @event CLIENT_GAMES_DIALOGCHANGED
         * @memberof Origin.events
         */
        CLIENT_DIALOGCHANGED: 'clientGamesDialogChanged',
        /**
         * This event is fired when we receive a signal that the initial base game update has been completed
         * @event CLIENT_GAMES_BASEGAMESUPDATED
         * @memberof Origin.events
         */
        CLIENT_GAMES_BASEGAMESUPDATED: 'clientGamesBaseGamesUpdated',
        /**
         * This event is fired when we receive a list of games that have either been added or removed. A list of added offerIds and a list of removed offerIds are passed with the event.
         * @event CLIENT_GAMES_LISTCHANGED
         * @memberof Origin.events
         */
        CLIENT_GAMES_LISTCHANGED: 'clientGamesListChanged',
        /**
         * This event is fired when we receive an updated progress status info.
         * @event CLIENT_GAMES_PROGRESSCHANGED
         * @memberof Origin.events
         */
        CLIENT_GAMES_PROGRESSCHANGED: 'clientGamesProgressChanged',
        /**
         * This event is fired when we receive a change in downloadqueue with a list of queue info for each entitlement
         * @event CLIENT_GAMES_DOWNLOADQUEUECHANGED
         * @memberof Origin.events
         */
        CLIENT_GAMES_DOWNLOADQUEUECHANGED: 'clientGamesDownloadQueueChanged',
        /**
         * This event is fired when the Origin client connection state has changed. A boolean is passed with the event. True means the client went online. False means the client went offline. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event CLIENT_ONLINESTATECHANGED
         * @memberof Origin.events
         */
        CLIENT_ONLINESTATECHANGED: 'clientOnlineStateChanged',
        /**
         * This event is fired when the settings data is ready.
         * @event SETTINGS_DATAREADY
         * @memberof Origin.events
         */
        SETTINGS_DATAREADY: 'settingsDataReady',
        /**
         * This event is fired when the settings data failed to retrieve.
         * @event SETTINGS_DATAFAILURE
         * @memberof Origin.events
         */
        SETTINGS_DATAFAILURE: 'settingsDataFailure',
        /**
         * This event is fired when the Origin client connection state has changed. A boolean is passed with the event. True means the client went online. False means the client went offline. [ONLY TRIGGERED IN EMBEDDED BROWSER]
         * @event LOCALE_CHANGED
         * @memberof Origin.events
         */
        LOCALE_CHANGED: 'localeChanged'
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
             * @memberof Origin.events
             */
            AUTH_TRIGGERLOGIN: 'authTriggerLogin'
        }
    };

    //for events that are meant to be used by integrators
    var events = new utils.Communicator();

    utils.mix(events, publicEventEnums);
    utils.mix(events, privateEventEnums);

    return events;
});
