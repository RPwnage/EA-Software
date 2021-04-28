(function() {
    'use strict';

    var PRESENCE_SHOW_UNAVAILABLE = 'UNAVAILABLE',
        PRESENCE_SHOW_ONLINE = 'ONLINE',
        VOICESTATE_INCOMING = 'INCOMING',
        VOICESTATE_OUTGOING = 'OUTGOING';

    /**
     * @ngdoc service
     * @name origin-components.service.ConversationService

     * @description
     *
     * ConversationService - stores our conversations, so that we can share it with other factories (ConversationFactory and RosterDataFactory for now)!
     */

    angular.module('origin-components').service('ConversationService', function() {
		var conversations= {};
        var timeoutHandles = {};

			this.getConversationById = function(id) {
                return conversations[id];
            };

			this.setConversationById = function(id, conversation) {
                conversations[id] = conversation;
            };

			this.removeConversationById = function(id) {
                delete conversations[id];
            };

			this.getConversations = function() {
                return conversations;
            };

            this.getTimeoutHandles = function() {
                return timeoutHandles;
            };

            this.getTimeoutHandlesByConversationId = function(conversationId) {
                if(!timeoutHandles[conversationId]) {
                    timeoutHandles[conversationId] = {};
                }
                return timeoutHandles[conversationId];
            };

            this.setTimeoutHandlesByConversationId = function(conversationId, timeoutHandleArray) {
                timeoutHandles[conversationId] = timeoutHandleArray;
            };

            this.getTimeoutHandlebyProductID = function(conversationId, productId) {
                if(!timeoutHandles[conversationId]) {
                    timeoutHandles[conversationId] = {};
                    timeoutHandles[conversationId][productId] = "";
                }
                return timeoutHandles[conversationId][productId];
            };

            this.setTimeoutHandlebyProductID = function(conversationId, productId, timeoutHandle) {
                if(!timeoutHandles[conversationId]) {
                    timeoutHandles[conversationId] = {};
                }
                timeoutHandles[conversationId][productId] = timeoutHandle;
            };

            this.clearAllTimeoutHandles = function() {
                for(var conversationId in timeoutHandles) {
                    var conversations = timeoutHandles[conversationId];
                    for(var productId in conversations) {
                        clearTimeout(conversations[productId]);
                    }
                }
                timeoutHandles = {};
            };

            this.clearTimeoutHandlesbyConversationId = function(conversationId) {
                var conversations = timeoutHandles[conversationId];
                if(conversations)  {
                    for(var productId in conversations) {
                        clearTimeout(conversations[productId]);
                    }
                }
                timeoutHandles[conversationId] = {};
            };

            this.clearTimeoutHandlebyProductID = function(conversationId, productId) {
                var conversations = timeoutHandles[conversationId];
                if(conversations)  {
                    if(conversations[productId]) {
                        clearTimeout(conversations[productId]);
                    }
                    timeoutHandles[conversationId][productId] = "";
                }
            };

			this.revokeInvitation  = function(id, myEvents) {
				this.getConversationById(id).invited = false;
				this.getConversationById(id).joinable = false;
				this.getConversationById(id).gameInvite = {};
                //To be safe clear the handle
                this.clearTimeoutHandlesbyConversationId(id);
				myEvents.fire('gameInviteRevoked:' + id);
			};

            this.ignoreInvitation  = function(id, myEvents) {
                this.getConversationById(id).invited = false;
                //To be safe clear the handle
                this.clearTimeoutHandlesbyConversationId(id);
                myEvents.fire('presenceChanged:' + id);
            };

            this.acceptInvitation  = function(id, myEvents) {
                this.getConversationById(id).invited = false;
                myEvents.fire('presenceChanged:' + id);
            };
		});


    function ConversationFactory(UserDataFactory, SocialDataFactory, RosterDataFactory, AppCommFactory, ChatWindowFactory, SocialHubFactory, AuthFactory, VoiceFactory, EventsHelperFactory, ConversationService, ObservableFactory, GamesDataFactory) {

        var activeConversationId,
            mostRecentChannelId,
            gameActivityMap = {},
            offline = false,
            myEvents = new Origin.utils.Communicator(),
            handles = [];

        /**
         *
         * @return New conversation object
         * @method
         */
        function createConversation(id, state, title, participants) {
            var newConversation = {
                id: id,
                state: state,
                title: title,
                participants: participants,
                unreadCount: 0,
                formattedEvents: [{ type: 'system', subtype: 'phishing' }],
                pendingText : '',
				invited: false,
				joinable: false,
                gameInvite: {}
            };

            // set up observables
            newConversation.remoteVoice = ObservableFactory.observe({isSupported: false});

            return newConversation;
        }

        /**
         *
         * @return Updates the conversation object's isVoiceSupportedRemotely flag depending on whether any of
         * the remote participants support voice chat.
         * @method
         */
        function updateVoiceSupportedRemotely(conversationId) {
            var doesAnyParticipantSupportVoice = false;
            var promiseArray  = [];

            if (!Origin.voice.supported()) {
                return;
            }

            for (var p = 0; p < ConversationService.getConversationById(conversationId).participants.length; ++p) {
                promiseArray.push(Origin.voice.isSupportedBy(ConversationService.getConversationById(conversationId).participants[p]));
            }

            Promise.all(promiseArray).then(function(results) {
                for(var i = 0; i < results.length; i++) {
                    if (results[i]) {
                        doesAnyParticipantSupportVoice = true;
                        break;
                    }
                }

                ConversationService.getConversationById(conversationId).remoteVoice.data.isSupported = doesAnyParticipantSupportVoice;
                ConversationService.getConversationById(conversationId).remoteVoice.commit();
            });
        }

        /**
         *
         * @return The last event in the conversation, or null if none exists
         * @method
         */
        function getMostRecentConversationEvent(conversationId) {
            return ConversationService.getConversationById(conversationId).formattedEvents.length > 0 ? ConversationService.getConversationById(conversationId).formattedEvents[ConversationService.getConversationById(conversationId).formattedEvents.length-1] : null;
        }

        /**
            *
            * @return latest timestamp or 0 if none found
            * @method
            */
        function findMostRecentTimestamp(conversationId) {
            // find latest timestamp event, return the time
            var latest = 0;
            if ((conversationId in ConversationService.getConversations()) && (ConversationService.getConversationById(conversationId).formattedEvents.length > 0)) {
                for (var x = ConversationService.getConversationById(conversationId).formattedEvents.length-1; x >= 0; --x) {
                    var event = ConversationService.getConversationById(conversationId).formattedEvents[x];
                    if (typeof event.time !== 'undefined') {
                        latest = event.time;
                        break;
                    }
                }
            }
            return latest;
        }

        /**
            *
            * @return Finds the most recent timestamp and compares it against time. If a new timestamp should be added it returns true
            * @method
            */
        function shouldAddTimestamp(conversationId, time) {
            var latestTimestamp = findMostRecentTimestamp(conversationId);
            return (latestTimestamp === 0 || (latestTimestamp + (60 * 1000)) < time);
        }

        /**
         *
         * @return {void}
         * @method
         */
        function addMessageToConversation(id, msg) {

            // Must have at least two periods, such as www.ea.com
            // URLs such as "amazon.com" (without www.) will need to have a http:// protocol appended in order to be linkified
            var validHostRegex = /^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.){2,}([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$/;

            // Special case to allow "ea.com" and "origin.com" as linkable URLs
            var eaOriginUrlsRegex = /(ea.com|origin.com)$/;


            var linkifyHtml = function (urlText) {
                //console.log("linkify: " + urlText);
                var result = urlText;
                var parsed = URI.parse(urlText);

                var href = "";

                // The protocol is specified in the urlText
                if (!!parsed.protocol) {
                    href = urlText;
                }
                else {
                    // Else, this URI is specified without a protocol. Check that the path contains a valid hostname, according to our definition, above.
                    if (!!parsed.path) {
                        var host = ( parsed.path.indexOf("/") > 0 ? parsed.path.slice(0, parsed.path.indexOf("/")) : parsed.path );
                        if (validHostRegex.test(host) || eaOriginUrlsRegex.test(host)) {
                            // Add http:// on to the link
                            href = "http://" + urlText;
                        }
                    }
                }

                if (href.length > 0) {
                    result = '<a class="otka user-provided" href="' + href + '">' + urlText + '</a>';
                }
                return result;
            };

            /**
             *
             * @return {void}
             * @method
             */
            function addFormattedMessage() {

                var addTimestamp = shouldAddTimestamp(id, msg.time);

                var decodedMessage = msg.message.replace(/&amp;/g, '&').replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&apos;/g, '\'').replace(/&quot;/g, '\"');

                var htmlDecodedMessage = URI.withinString(decodedMessage, function(url) {
                    var linkified = linkifyHtml(url);
                    return linkified;
                }, {
                    ignoreHtml: false,
                    // valid "scheme://" or "www.", or ending with "origin.com" or "ea.com"
                    start: /\b(?:(https?:\/\/)|www\.|[A-Za-z0-9]*\.?origin\.com|[A-Za-z0-9]*\.?ea\.com)/gi,
                    // trim trailing punctuation captured by end RegExp: add encoded symbols
                    trim: /([`!()\[\]{};,'"<>]|&amp;|&quot;|&gt;|&lt;)+?.*$/
                });

                var monologueEvent = {
                    type: 'monologue',
                    time: (addTimestamp ? msg.time : undefined),
                    from: msg.from,
                    messages: [ htmlDecodedMessage ]
                };

                var mostRecentEvent = getMostRecentConversationEvent(id);
                if (mostRecentEvent) {
                    // Is most recent event a monologue?
                    if (mostRecentEvent.type === 'monologue') {
                        if (Number(mostRecentEvent.from) === Number(msg.from)) {
                            if (addTimestamp) {
                                // need to add a timestamp, so start a new monologue event
                                ConversationService.getConversationById(id).formattedEvents.push(monologueEvent);
                            } else {
                                // append message to existing monologue
                                mostRecentEvent.messages.push( htmlDecodedMessage );
                            }
                        } else {
                            // not from the same sender, so start a new monologue event
                            ConversationService.getConversationById(id).formattedEvents.push(monologueEvent);
                        }
                    } else {
                        // most recent event is not a monologue, so start a new monologue event
                        ConversationService.getConversationById(id).formattedEvents.push(monologueEvent);
                    }

                } else {
                    // This is the first event, so add the new monologue event
                    ConversationService.getConversationById(id).formattedEvents.push(monologueEvent);
                }
            }

            // Add the message to the formatted list
            addFormattedMessage();

            //console.log('formattedEvents: ' + JSON.stringify(ConversationService.getConversationById(id).formattedEvents));

            //console.log('formattedEvents count: ' + ConversationService.getConversationById(id).formattedEvents.length);

            // Update the unread count
            ConversationService.getConversationById(id).unreadCount++;

            // Notify that a new message has been added
            myEvents.fire('addMessage', id.toString());

            // Notify that the conversation history has been updated
            var fromMe = (Number(msg.from) === Number(Origin.user.userPid())) ? true : false;
            myEvents.fire('historyUpdated', id.toString(), fromMe);
        }

        /**
         *
         * @return {void}
         * @method
         */
        function addMessageEvent(msg, conversationId) {

            if (conversationId === 0) {
                return;
            }

            // This is the first message in the conversation
            if (typeof ConversationService.getConversationById(conversationId) === 'undefined') {

                var fromId = Number(msg.from);
                if (fromId === Number(Origin.user.userPid())) {

                    // Carbon copy of a message from us
                    var conversationIdStr = conversationId.toString();
                    if (conversationIdStr.indexOf('gac_') === 0) {

                        // Group chat
                        // Join the group conversation
                        joinConversation(conversationId);

                        // Need to wait for the conversation to open before we add the message from ourselves
                        // This is pretty hacky
                        setTimeout(function () {
                            addMessageToConversation(conversationId, msg);
                        }, 2000);
                    }
                    else {
                        // 1:1
                        fromId = conversationId;
                        ConversationService.setConversationById(conversationId, createConversation(conversationId, 'ONE_ON_ONE', '', [fromId]));
                        updateVoiceSupportedRemotely(conversationId);
                        addMessageToConversation(conversationId, msg);
                        RosterDataFactory.getFriendInfo(fromId).then(function (friend) {
                            ConversationService.getConversationById(conversationId).title = friend.originId;
                            openConversation(conversationId.toString());
                        });
                    }
                }
                else {
                    ConversationService.setConversationById(conversationId, createConversation(conversationId, 'ONE_ON_ONE', '', [fromId]));
                    updateVoiceSupportedRemotely(conversationId);
                    addMessageToConversation(conversationId, msg);
                    RosterDataFactory.getFriendInfo(msg.from).then(function (friend) {
                        ConversationService.getConversationById(conversationId).title = friend.originId;
                        openConversation(conversationId.toString());
                    });
                }
            } else {
                addMessageToConversation(conversationId, msg);
            }
        }

         /**
         * called on XMPP message events
         * @return {void}
         * @method
         */
        function onXmppIncomingMessage(xmppMsg) {
            var fromId;

            // console.log('ConversationFactory: onXmppIncomingMessage ' + xmppMsg.msgBody);
            // console.log('ConversationFactory: onXmppIncomingMessage ' + xmppMsg.time);
            // console.log('ConversationFactory: onXmppIncomingMessage ' + xmppMsg.chatState);
            // console.log('ConversationFactory: onXmppIncomingMessage ' + xmppMsg.jid);
            // console.log('ConversationFactory: onXmppIncomingMessage ' + xmppMsg.to);

            if (!!xmppMsg.chatState && xmppMsg.chatState.length){
                if (xmppMsg.jid.indexOf('gac_') === 0) {
                    // multi user chat - do nothing
                }
                else {
                    // update typing indicator
                    var conversationId = Number(xmppMsg.jid.split('@')[0]);
                    myEvents.fire('typingStateUpdated:' + conversationId, xmppMsg.chatState);
                }
            }

            // Strip out any HTML tags
            var strippedMsg = xmppMsg.msgBody;
            strippedMsg = strippedMsg.split('&lt;').join('<');
            strippedMsg = strippedMsg.split('&gt;').join('>');
            strippedMsg = strippedMsg.replace(/<[^>]*>/g, '');
            if (strippedMsg.length) {

                fromId = Number(xmppMsg.jid.split('@')[0]);
                if (fromId === Origin.user.userPid()) {

                    // This is a carbon of a message from ourselves

                    if (xmppMsg.to.indexOf('gac_') === 0) {
                        // Carbon group chat message

                        // See if we have this conversation open already.
                        // If so, simply eat the message as we'll receive it normally as well
                        // If not, open the conversation and add the message

                        if (typeof ConversationService.getConversationById(xmppMsg.to) === 'undefined') {
                            var message = {
                                message: strippedMsg,
                                time: xmppMsg.time,
                                from: fromId,
                                type: 'message'
                            };

                            addMessageEvent(message, xmppMsg.to.split('/')[0]);
                        }

                    }
                    else {
                        // Carbon 1:1 message
                        var toId = Number(xmppMsg.to.split('@')[0]);
                        addMessageEvent({
                            message: strippedMsg,
                            time: xmppMsg.time,
                            from: fromId,
                            type: 'message'
                        }, toId);
                    }
                }
                else {

                    // Normal received message from another user

                    if (xmppMsg.jid.indexOf('gac_') === 0) {

                        var originId = xmppMsg.from.split('/')[1];

                        SocialDataFactory.getGroupMemberInfo(originId).then(function(id) {
                            // Multi-user chat
                            var message = {
                                message: strippedMsg,
                                time: xmppMsg.time,
                                from: id,
                                type: 'message'
                            };

                            addMessageEvent(message, xmppMsg.jid.split('/')[0]);
                        });


                    }
                    else {
                        // 1:1 message
                        addMessageEvent({
                            message: strippedMsg,
                            time: xmppMsg.time,
                            from: fromId,
                            type: 'message'
                        }, fromId);
                    }
                }
            }
        }

        /**
         *
         * @return {void}
         * @method
         */
        function addPresenceToConversation(id, presence) {

            /**
             *
             * @return {void}
             * @method
             */
            function addFormattedPresence() {
                var mostRecentEvent = getMostRecentConversationEvent(id);
                if (mostRecentEvent && (mostRecentEvent.type === 'system') && (mostRecentEvent.subtype === 'presence')) {
                    // update existing presence event
                    mostRecentEvent.presence = presence.presence;
                }
                else {
                    // new presence event- add to bottom of formattedEvents
                    var addTimestamp = shouldAddTimestamp(id, presence.time);
                    var formattedPresence = {
                        type: presence.type,
                        subtype: presence.subtype,
                        time: addTimestamp ? presence.time : undefined,
                        from: presence.from,
                        presence: presence.presence
                    };
                    ConversationService.getConversationById(id).formattedEvents.push(formattedPresence);
                }

                mostRecentEvent = getMostRecentConversationEvent(id);
                //Need to stop the composing animation if this user goes offline
                if (mostRecentEvent.presence === 'offline' || mostRecentEvent.presence === 'unavailable') {
                    myEvents.fire('typingStateUpdated:' + id, 'ACTIVE');
                }
            }

            addFormattedPresence();

            setTimeout(function () {
                myEvents.fire('historyUpdated', id.toString(), false);
            }, 50);


            //console.log('formattedEvents: ' + JSON.stringify(ConversationService.getConversationById(id).formattedEvents));

            //console.log('formattedEvents count: ' + ConversationService.getConversationById(id).formattedEvents.length);
        }

        /**
         *
         * @return array of conversation IDs
         * @method
         */
        function getConversationIdArrayFromPresence(presence) {
            var IDs = [];

            for (var id in ConversationService.getConversations()) {
                var conv = ConversationService.getConversationById(id);
                if (conv.participants.indexOf(presence.from) >= 0) {
                    IDs.push(id);
                }
            }
            return IDs;
        }


        /**
         *
         * @return {void}
         * @method
         */
        function addPresenceEventOneToOne(presence) {
            //console.log('addPresenceEventOneToOne: ' + presence);
            var conversationIdArray = getConversationIdArrayFromPresence(presence);

            //console.log('conversationIdArray: ' + conversationIdArray.length);
            if (conversationIdArray.length > 0) {
                $.each(conversationIdArray, function (index, id) {
                    if (ConversationService.getConversationById(id).state === 'ONE_ON_ONE') {
                        addPresenceToConversation(id, presence);
                    }
                });
            }
        }

        /**
         *
         * @return {void}
         * @method
         */
        function addPresenceEventMulti(presence) {

            var conversationId = presence.id;
            delete presence.id;

            var addTimestamp = shouldAddTimestamp(conversationId, presence.time);

            /**
             * Maintain the participants array
             * @return {void}
             * @method
             */
            function updateParticipants() {
                var participantsArrayIndex = ConversationService.getConversationById(conversationId).participants.indexOf(presence.from);
                if ((presence.subtype === 'participant_enter') && (participantsArrayIndex < 0)) {
                    ConversationService.getConversationById(conversationId).participants.push( presence.from );
                    updateVoiceSupportedRemotely(conversationId);
                    myEvents.fire('updateParticipantList');
                }

                if ((presence.subtype === 'participant_exit') && (participantsArrayIndex >= 0)) {
                    ConversationService.getConversationById(conversationId).participants.splice(participantsArrayIndex, 1);
                    updateVoiceSupportedRemotely(conversationId);
                    myEvents.fire('updateParticipantList');
                }
            }

            if (typeof ConversationService.getConversationById(conversationId)=== 'undefined') {
                // New conversation
                SocialDataFactory.getGroupName(conversationId).then(function(name) {

                    ConversationService.setConversationById(conversationId, createConversation(conversationId, 'MULTI_USER', name, [Number(presence.from)]));
                    updateVoiceSupportedRemotely(conversationId);

                    if ((presence.subtype === 'participant_enter') || (presence.subtype === 'participant_exit')) {
                        if (presence.from !== Origin.user.userPid()) {
                            var formattedPresence = {
                                type: presence.type,
                                subtype: presence.subtype,
                                time: addTimestamp ? presence.time : undefined,
                                from: [ presence.from ]
                            };
                            ConversationService.getConversationById(conversationId).formattedEvents.push(formattedPresence);
                            myEvents.fire('historyUpdated', conversationId.toString(), false);
                        }
                    }

                    updateParticipants();
                    ChatWindowFactory.raiseConversation(conversationId.toString());

                    //console.log('ConversationService.getConversationById('+conversationId+']: ' + JSON.stringify(ConversationService.getConversationById(conversationId)));
                });
            }
            else {
                // Existing conversation
                if ((presence.subtype === 'participant_enter') || (presence.subtype === 'participant_exit')) {
                    if (presence.from !== Origin.user.userPid()) {
                        var mostRecentEvent = getMostRecentConversationEvent(conversationId);
                        if (mostRecentEvent && (mostRecentEvent.type === presence.type) && (mostRecentEvent.subtype === presence.subtype)) {
                            // Update existing presence message
                            mostRecentEvent.from.unshift(presence.from);
                        }
                        else {
                            // Create a new presence message
                            var formattedPresence = {
                                type: presence.type,
                                subtype: presence.subtype,
                                time: addTimestamp ? presence.time : undefined,
                                from: [ presence.from ]
                            };
                            ConversationService.getConversationById(conversationId).formattedEvents.push(formattedPresence);
                        }
                        myEvents.fire('historyUpdated', conversationId.toString(), false);
                    }
                }

                updateParticipants();
                ChatWindowFactory.raiseConversation(conversationId.toString());
                //console.log('ConversationService.getConversationById('+conversationId+']: ' + JSON.stringify(ConversationService.getConversationById(conversationId)));
            }
        }

        function lastEventSubtype(conversation) {
            var last = conversation.formattedEvents.length - 1,
                event = conversation.formattedEvents[last];

            if (event.type === 'system') {
                return event.subtype;
            }
        }

        function addVoiceEvent(conversationId, type, errorCode) {
            RosterDataFactory.getFriendInfo(conversationId).then(function (friend) {
                var conversation = ConversationService.getConversationById(conversationId),
                    lastSubtype = lastEventSubtype(conversation);

                // skip the 'Call Ended' and 'did not answer' messages if the previous was a 'connect error' message
                if ((type === 'call_noanswer' || type === 'call_ended') && lastSubtype === 'call_connect_error') {
                    return;
                }

                // skip the "Call Ended" message if the previous was an "Inactivity" message
                if (type === 'call_ended' && lastSubtype === 'inactivity') {
                    return;
                }

                // remove previous "Call Ended" message
                if (type === 'call_noanswer' || type === 'call_missed') {
                    if ( (lastSubtype === 'call_ended') || (lastSubtype === 'call_ended_incoming') ) {
                        conversation.formattedEvents.splice(-1);
                    }
                }

                // remove previous 'Unmuted' message
                if (type === 'local_muted') {
                    if (lastSubtype === 'local_unmuted') {
                        conversation.formattedEvents.splice(-1);
                    }
                }

                // Create new voice message
                var eventTime = Date.now();
                var addTimestamp = shouldAddTimestamp(conversationId, eventTime);
                var formattedVoiceMessage = {
                    type: 'system',
                    from: friend.originId,
                    channelId: mostRecentChannelId,
                    errorCode: errorCode,
                    subtype: type,
                    time: addTimestamp ? eventTime : undefined
                };
                conversation.formattedEvents.push(formattedVoiceMessage);

                // Add badge for missed calls
                if (type === 'call_missed') {
                    conversation.unreadCount++;
                    myEvents.fire('addMessage', conversationId.toString());
                }

                myEvents.fire('historyUpdated', conversationId.toString(), false);
            });
        }

        /**
         *
         * @return {void}
         * @method
         */
        function onXmppPresenceChanged(xmppPresence) {
			console.log('onXmppPresenceChanged');
/*
            console.log('ConversationFactory: onXmppPresenceChanged presenceType:' + xmppPresence.presenceType);
            console.log('ConversationFactory: onXmppPresenceChanged show:' + xmppPresence.show);
            console.log('ConversationFactory: onXmppPresenceChanged gameActivity:' + JSON.stringify(xmppPresence.gameActivity));
            console.log('ConversationFactory: onXmppPresenceChanged from:' + xmppPresence.from);
            console.log('ConversationFactory: onXmppPresenceChanged jid:' + xmppPresence.jid);
            console.log('ConversationFactory: onXmppPresenceChanged to:' + xmppPresence.to);
*/

            // Ignore all presence events while we are in offline mode
            if (offline) {
                return;
            }

            var eventTime = Date.now(),
                nucleusId = 0;

            if (xmppPresence.jid.indexOf('gac_') === 0) {
                // This is multi user chat

                if (!!xmppPresence.from) {
                    nucleusId = xmppPresence.from.split('@')[0];
                    if (nucleusId.length > 0) {
                        nucleusId = Number(nucleusId);
                    }
                }

                var subtype = '';
                if (xmppPresence.show === PRESENCE_SHOW_ONLINE && typeof xmppPresence.presenceType === 'undefined') {
                    subtype = 'participant_enter';
                } else if (xmppPresence.show === PRESENCE_SHOW_ONLINE && xmppPresence.presenceType === 'UNAVAILABLE') {
                    subtype = 'participant_exit';
                }

                if (subtype === 'participant_exit' && nucleusId === Origin.user.userPid()) {
                    // current user leaves conversation. Nothing to do
                } else {
                    addPresenceEventMulti({
                        type: 'system',
                        subtype: subtype,
                        id: xmppPresence.jid.slice(0, xmppPresence.jid.indexOf('/')),
                        time: eventTime,
                        from: nucleusId
                    });
                }


            } else {
                // This is 1:1
                var gameRelated = false;

                nucleusId = Number(xmppPresence.jid.slice(0, xmppPresence.jid.indexOf('@')));

                var user = RosterDataFactory.getRosterFriend(nucleusId);
                if (!!user && user.subState === 'BOTH') {
					myEvents.fire('presenceChanged:' + nucleusId);
					// See if this is a user starting or stopping a game
                    if (xmppPresence.gameActivity && xmppPresence.gameActivity.title !== undefined && xmppPresence.gameActivity.title !== '') {
                        // User started playing a game
                        gameActivityMap[nucleusId] = xmppPresence.gameActivity;
                        gameRelated = true;

						updateJoinableState(nucleusId);
					}
                    else if ((typeof gameActivityMap[nucleusId] !== 'undefined') && gameActivityMap[nucleusId].title !== undefined && gameActivityMap[nucleusId].title !== '') {
                        // User stopped playing a game
                        delete gameActivityMap[nucleusId];
                        gameRelated = true;

						updateJoinableState(nucleusId);
                    }

                    if (!gameRelated) {
                        addPresenceEventOneToOne({
                            type: 'system',
                            subtype: 'presence',
                            time: eventTime,
                            from: nucleusId,
                            presence: ((xmppPresence.presenceType === 'UNAVAILABLE') ? 'offline' : xmppPresence.show.toLowerCase())
                        });
                        // If an incoming caller has gone offline, or if the target of an outgoing call has gone offline, end the call.
                        if (Origin.voice.supported() && xmppPresence.show === PRESENCE_SHOW_UNAVAILABLE) {
                            var callInfo = VoiceFactory.voiceCallInfoForConversation(nucleusId);
                            if (callInfo && (callInfo.state === VOICESTATE_INCOMING || callInfo.state === VOICESTATE_OUTGOING)) {
                                Origin.voice.leaveVoice(nucleusId);
                            }
                        }
                    }
                }
            }
        }

        function revokeInvitation(conversationId) {
            var conversation = ConversationService.getConversationById(conversationId);
            var eventTime = Date.now();
            var addTimestamp = shouldAddTimestamp(conversationId, eventTime);
            var formattedPresence = {
                type: 'system',
                subtype: 'revoked_invite',
                time: addTimestamp ? eventTime : undefined,
                from: conversation.title,
                productId: conversation.gameInvite.productId,
                gameName: ''
            };
            if (!!formattedPresence.productId) {
                GamesDataFactory.getCatalogInfo([formattedPresence.productId]).then(function(catalogInfo) {
                    formattedPresence.gameName = catalogInfo[formattedPresence.productId].i18n.displayName;
                    ConversationService.getConversationById(conversationId).formattedEvents.push(formattedPresence);
                    myEvents.fire('historyUpdated', conversationId.toString(), false);
                });
                ConversationService.revokeInvitation(conversationId, myEvents);                
            }
        }

		function updateJoinableState(conversationId) {
			// update joinable & invite state for our conversation
            var updateConversation = ConversationService.getConversationById(conversationId);
            if(updateConversation) {
			    if ((typeof gameActivityMap[conversationId] !== 'undefined') && gameActivityMap[conversationId].title !== undefined && gameActivityMap[conversationId].title !== '') {
				// Joinable state?
				    if (gameActivityMap[conversationId].joinable || (gameActivityMap[conversationId].joinableInviteOnly && ConversationService.getConversationById(conversationId).invited)) {
					   updateConversation.joinable = true;
                       ConversationService.setConversationById(conversationId, updateConversation);
					   myEvents.fire('gameStateChanged:' + conversationId);
				    } else {
					   updateConversation.joinable = false;
					   updateConversation.invited = false;
                       ConversationService.setConversationById(conversationId, updateConversation);
					   revokeInvitation(conversationId);
				    }
			    } else {
				    updateConversation.joinable = false;
				    updateConversation.invited = false;
                    ConversationService.setConversationById(conversationId, updateConversation);
				    revokeInvitation(conversationId);
			    }
            }
		}

		function onXmppGameInviteReceived(inviteObj) {
            console.log('onXmppGameInviteReceived');
			// Game invite received open a conversation
            var nucleusId = inviteObj.from.split('@')[0];
            var conversationId = Number(nucleusId);

            RosterDataFactory.getFriendInfo(nucleusId).then(function (friend) {
                var conversation,  isNewConversation = openConversationNoRaise(nucleusId, friend);

                if (isNewConversation) {
                    raiseConversation(nucleusId);
                }

                conversation = ConversationService.getConversationById(conversationId);

                // Add the invite object
                conversation.gameInvite = inviteObj;

                // Update the unread count
                conversation.unreadCount++;

                // In the case that the user is not a mutual friend the invite is the first presence update we get from that user.
                if(friend.subState !== 'MUTUAL')
                {
                    var newPresence = {
                        jid: inviteObj.from,
                        show: PRESENCE_SHOW_ONLINE,
                        groupActivity: {
                            groupId: inviteObj.groupId,
                            groupName: inviteObj.groupName
                        },
                        gameActivity: {
                            gamePresence: '',
                            gameTitle: 'Some Game',
                            joinable: true,
                            joinableInviteOnly: false,
                            multiplayerId: inviteObj.multiplayerId,
                            productId: inviteObj.productId,
                            richPresence: '',
                            title: 'Some Game',
                            twitchPresence: ''
                        }
                    };


                    RosterDataFactory.updatePresence(conversationId, newPresence);

                    // Update the presence again, when we get the game name.
                    GamesDataFactory.getCatalogInfo([inviteObj.productId])
                        .then(function(obj){
                            var offer = obj[inviteObj.productId];
                            if(offer !== undefined) {
                                newPresence.gameActivity.title = offer.i18n.displayName;
                                newPresence.gameActivity.gameTitle = offer.i18n.displayName;

                                RosterDataFactory.updatePresence(conversationId, newPresence);
                            }
                        });

                    gameActivityMap[conversationId] = friend.presence.gameActivity;
                }

                // Notify that a new message has been added
                myEvents.fire('addMessage', conversationId.toString());

				// Invited by a friend?
				if (conversation.state === 'ONE_ON_ONE') {
					if (!!conversation.gameInvite.from && !!conversation.gameInvite.productId && !!conversation.gameInvite.multiplayerId) {
                        conversation.invited = true;
                        //Clear all existing timeouts for this conversation so that our new timeout isn't closed early.
                        ConversationService.clearTimeoutHandlesbyConversationId(conversationId);
						// Set a timeout for a "join a game invitation" - 60 minutes
						ConversationService.setTimeoutHandlebyProductID(conversationId, conversation.gameInvite.productId,
                            setTimeout(function () {revokeInvitation(conversationId);
						    }, 60*60*1000));
					}
				}
				updateJoinableState(conversationId);
			});
		}

        function onClientOnlineStateChanged(onlineObj) {
            var id, conv,
                online = (onlineObj && onlineObj.isOnline);

            if (online) {
                // we just logged back on -
                // we're going to get flooded with presence events
                // supress showing those in the conversations
                // This mimics the old "chat settled" code from 9.x
                setTimeout(function () {
                    offline = false;
                }, 10000);
            }
            else {
                offline = true;
            }

            // Remove previous "you are offline" system message from all conversations
            for (id in ConversationService.getConversations()) {
                conv = ConversationService.getConversationById(id);
                for (var x = conv.formattedEvents.length - 1; x >= 0; --x) {
                    var event = conv.formattedEvents[x];
                    if (event.subtype === 'offline_mode') {
                        conv.formattedEvents.splice(x, 1);
                        myEvents.fire('historyUpdated', id.toString(), false);
                    }
                }
            }

            if (!online) {
                // Add a "you are offline" system message to all conversations
                // Stop all composing animations for all conversations too
                var eventTime = Date.now();

                for (id in ConversationService.getConversations()) {
                    conv = ConversationService.getConversationById(id);
                    var addTimestamp = shouldAddTimestamp(id, eventTime);
                    var formattedEvent = {
                        type: 'system',
                        subtype: 'offline_mode',
                        time: addTimestamp ? eventTime : undefined
                    };
                    conv.formattedEvents.push(formattedEvent);
                    myEvents.fire('historyUpdated', id.toString(), false);
                    myEvents.fire('typingStateUpdated:' + id, 'ACTIVE');
                }

                if (Origin.voice.supported()) {
                    // End any active call (includes outgoing call) or incoming call.
                    Origin.voice.leaveVoice(VoiceFactory.activeVoiceConversationId());
                    Origin.voice.leaveVoice(VoiceFactory.incomingVoiceConversationId());
                }
            }

        }

        function onXmppConnect() {
            onClientOnlineStateChanged({isLoggedIn: true,
                                        isOnline:true});
        }

        function onXmppDisconnect() {
            onClientOnlineStateChanged({isLoggedIn: true,
                                        isOnline:false});
        }

        function handleRemovedFriend(nucleusId) {
            // If we unfriend a user with an open conversation,
            // we never want the last event to be a presence event.
            var conv = ConversationService.getConversationById(nucleusId);
            if (!!conv) {
                var event = getMostRecentConversationEvent(conv.id);
                if (event && event.type === 'system' && event.subtype === 'presence') {
                    conv.formattedEvents.pop();
                    myEvents.fire('historyUpdated', conv.id.toString(), false);
                }
            }
        }

        /**
         *
         * @return {void}
         * @method
         */
        function joinConversation(roomJid) {
            Origin.xmpp.joinRoom(roomJid, Origin.user.originId());
        }

        function setActiveConversationId(id) {
            if (id !== activeConversationId) {
                //console.log('Setting active conversation to: ', id);
                activeConversationId = id;
                myEvents.fire('activeConversationIdChanged', activeConversationId.toString());
            }
        }

        function raiseConversation(conversationId) {
            ChatWindowFactory.raiseConversation(conversationId.toString());
            if (SocialHubFactory.isWindowPoppedOut() && !ChatWindowFactory.isWindowPoppedOut()) {
                ChatWindowFactory.popOutWindow();
            }
            // If we are embedded need to trigger an OIG window too
            if (Origin.client.isEmbeddedBrowser() && Origin.client.oig.IGOIsAvailable()) {
                Origin.client.oig.openIGOConversation();
            }
        }

        function openConversation(nucleusId) {
            var conversationId = nucleusId;
            if (typeof ConversationService.getConversationById(conversationId) === 'undefined') {
                RosterDataFactory.getFriendInfo(nucleusId).then(function(friend) {
                    var initializedConversation = createConversation(conversationId, 'ONE_ON_ONE', friend.originId, [Number(nucleusId)]);
                    initializedConversation.joinable = friend.presence.gameActivity.joinable;
                    ConversationService.setConversationById(conversationId, initializedConversation);
                    updateVoiceSupportedRemotely(conversationId);

                    // Add an initial "offline" message
                    if (friend.presence.show === PRESENCE_SHOW_UNAVAILABLE) {
                        addPresenceToConversation(conversationId, {
                            type: 'system',
                            subtype: 'presence',
                            time: Date.now(),
                            from: nucleusId,
                            presence: 'offline'
                        });
                    }

                    // always raise the conversation
                    raiseConversation(conversationId);
                    // The this to the active conversation since we just opened it
                    setActiveConversationId(conversationId);
                });
            }
            else {
                 // always raise the conversation
                 raiseConversation(conversationId);
                 // The this to the active conversation since we just opened it
                 setActiveConversationId(conversationId);
            }
        }

        function openConversationNoRaise(nucleusId, friend) {
            var isNewConversation = false,
                conversationId = nucleusId;

            if (typeof ConversationService.getConversationById(conversationId) === 'undefined') {
                ConversationService.setConversationById(conversationId, createConversation(conversationId, 'ONE_ON_ONE', friend.originId, [Number(nucleusId)]));
                updateVoiceSupportedRemotely(conversationId);

                // Add an initial "offline" message
                if (friend.presence.show === PRESENCE_SHOW_UNAVAILABLE) {
                    addPresenceToConversation(conversationId, {
                        type: 'system',
                        subtype: 'presence',
                        time: Date.now(),
                        from: nucleusId,
                        presence: 'offline'
                    });
                }

                isNewConversation = true;
            }

            return isNewConversation;
        }

        function showInProgressCallBanner(nucleusId) {
        	RosterDataFactory.getFriendInfo(nucleusId).then(function(friend) {
        		var newconv = openConversationNoRaise(nucleusId, friend);
        		if(!newconv) {
        			raiseConversation(nucleusId);
        			setActiveConversationId(nucleusId);
        			myEvents.fire('showOutgoingCallBanner:' + nucleusId);
        		}
        		else {
        			if (typeof ConversationService.getConversationById(nucleusId) === 'undefined') {
            	        var initializedConversation = createConversation(nucleusId, 'ONE_ON_ONE', friend.originId, [Number(nucleusId)]);
        	            initializedConversation.joinable = friend.presence.gameActivity.joinable;
    	                ConversationService.setConversationById(nucleusId, initializedConversation);
	                    updateVoiceSupportedRemotely(nucleusId);

                    	// Add an initial "offline" message
                    	if (friend.presence.show === PRESENCE_SHOW_UNAVAILABLE) {
                      		addPresenceToConversation(nucleusId, {
                        	   	type: 'system',
                           		subtype: 'presence',
                           		time: Date.now(),
                           		from: nucleusId,
                            	presence: 'offline'
	                    });
                    }

                    // always raise the conversation
                    raiseConversation(nucleusId);
                    // The this to the active conversation since we just opened it
                    setActiveConversationId(nucleusId);
                    myEvents.fire('showOutgoingCallBanner:' + nucleusId);
        			}
        		}
        	});
        }

        function hideInProgressCallBanner(nucleusId) {
        	if (typeof ConversationService.getConversationById(nucleusId) !== 'undefined') {
            	myEvents.fire('hideInProgressCallBanner:' + nucleusId);
        	}
        }

        function inProgressCallBannerHidden(nucleusId) {
            if (typeof ConversationService.getConversationById(nucleusId) === 'undefined') {
            	myEvents.fire('inProgressCallBannerHidden:' + nucleusId);
            }
        }

        function closeConversationById(id) {
            ConversationService.removeConversationById(id);

            if (id.indexOf('gac_')===0) {
                Origin.xmpp.leaveRoom(id, Origin.user.originId());
            }
            if (Origin.voice.supported()) {
                if (id === VoiceFactory.activeVoiceConversationId()) {
                    Origin.voice.leaveVoice(id);
                }
            }

            myEvents.fire('conversationClosed', id.toString());
        }

        function closeAllConversations() {
            for (var id in ConversationService.getConversations()) {
                closeConversationById(id);
            }
        }

        function markConversationReadById(id) {
            if (angular.isUndefined(id) || id === '' || id === '0') {
                return;
            }
            ConversationService.getConversationById(id).unreadCount = 0;
            myEvents.fire('conversationRead', id);
        }

        function markActiveConversationRead() {
            markConversationReadById(activeConversationId);
        }

        function getNumConversations() {
            return Object.keys(ConversationService.getConversations()).length;
        }

        function openConversationForIncomingVoiceCall(nucleusId) {
            RosterDataFactory.getFriendInfo(nucleusId).then(function(friend) {
                var isNewConversation;

                // no chat windows open
                if (getNumConversations() === 0) {
                    openConversation(nucleusId);
                    isNewConversation = true;
                }
                else {
                    isNewConversation = openConversationNoRaise(nucleusId, friend);
                    if (isNewConversation) {
                        raiseConversation(nucleusId);
                    }
                }

                myEvents.fire('incomingVoiceCall', nucleusId, friend, isNewConversation);
            });
        }

        function handleVoiceCallEvent(voiceCallEventObj) {
            var event = voiceCallEventObj.event,
                nucleusId = voiceCallEventObj.nucleusId,
                channelId = voiceCallEventObj.channelId,
                errorCode = voiceCallEventObj.errorCode;

            switch (event) {
                case 'INCOMING':
                    openConversationForIncomingVoiceCall(nucleusId);
                    break;

                case 'STARTED':
                    raiseConversation(nucleusId);
                    addVoiceEvent(nucleusId, 'call_started');
                    mostRecentChannelId = channelId;
                    break;

                case 'ENDED':
                    addVoiceEvent(nucleusId, 'call_ended');
                    break;

                case 'NOANSWER':
                    addVoiceEvent(nucleusId, 'call_noanswer');
                    break;

                case 'MISSED':
                    addVoiceEvent(nucleusId, 'call_missed');
                    break;

                case 'LOCAL_MUTED':
                    addVoiceEvent(nucleusId, 'local_muted');
                    break;

                case 'LOCAL_UNMUTED':
                    addVoiceEvent(nucleusId, 'local_unmuted');
                    break;

                case 'INACTIVITY':
                    addVoiceEvent(nucleusId, 'inactivity');
                    break;

                case 'ENDED_UNEXPECTEDLY':
                    addVoiceEvent(nucleusId, 'call_ended_unexpectedly');
                    break;

                case 'ENDED_INCOMING':
                    addVoiceEvent(nucleusId, 'call_ended_incoming');
                    break;

                case 'CONNECT_ERROR':
                    addVoiceEvent(nucleusId, 'call_connect_error', errorCode);
                    break;
            }
        }

        function addDeviceChangedEvent(conversationId, deviceName) {
            var conversation = ConversationService.getConversationById(conversationId),
                lastSubtype = lastEventSubtype(conversation);

            // skip multiple 'Your microphone is now unplugged' message
            if (lastSubtype === 'device_changed' && deviceName === undefined) {
                return;
            }

            // Create new voice message
            var eventTime = Date.now();
            var addTimestamp = shouldAddTimestamp(conversationId, eventTime);
            var formattedVoiceMessage = {
                type: 'system',
                device: deviceName,
                subtype: 'device_changed',
                time: addTimestamp ? eventTime : undefined
            };
            ConversationService.getConversationById(conversationId).formattedEvents.push(formattedVoiceMessage);

            myEvents.fire('historyUpdated', conversationId.toString(), false);
        }

        function handleVoiceInputDeviceChanged(deviceName) {
            for (var id in ConversationService.getConversations()) {
                addDeviceChangedEvent(id, deviceName);
            }
        }

        function handleDesktopDockIconClicked() {
            markActiveConversationRead();
        }

        function init() {
            //console.log('ConversationFactory: init()');

            // listen for incoming Xmpp messages
            handles[handles.length] = Origin.events.on(Origin.events.XMPP_INCOMINGMSG, onXmppIncomingMessage);

            // listen for XMPP presence changes
            handles[handles.length] = Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, onXmppPresenceChanged);

            // listen for incoming game invitations
            handles[handles.length] = Origin.events.on(Origin.events.XMPP_GAMEINVITERECEIVED, onXmppGameInviteReceived);

            //listen for connection change (for embedded)
            handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

            // listen for xmpp connect/disconnect
            handles[handles.length] = RosterDataFactory.events.on('xmppConnected', onXmppConnect);
            handles[handles.length] = RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);

            // listen for join Conversation events
            handles[handles.length] = SocialDataFactory.events.on('joinConversation', joinConversation);

            // Listen for unfriending
            handles[handles.length] = RosterDataFactory.events.on('removedFriend', handleRemovedFriend);
            window.addEventListener('unload', destroy);

            // listen for voice call events
            handles[handles.length] = VoiceFactory.events.on('voiceCallEvent', handleVoiceCallEvent);

            // listen for voice input device changed event
            handles[handles.length] = VoiceFactory.events.on('voiceInputDeviceChanged', handleVoiceInputDeviceChanged);

            // listen for desktop events
            handles[handles.length] = Origin.events.on(Origin.events.CLIENT_DESKTOP_DOCKICONCLICKED, handleDesktopDockIconClicked);
        }

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }

        return {

            init: init,
            events: myEvents,
            setActiveConversationId: setActiveConversationId,

			revokeInvitation: function(conversationId) {
                revokeInvitation(conversationId);
			},

            ignoreInvitation: function(conversationId) {
                ConversationService.ignoreInvitation(conversationId, myEvents);
            },

            acceptInvitation: function(conversationId) {
                ConversationService.acceptInvitation(conversationId, myEvents);
            },

			getConversations: function() {
                return ConversationService.getConversations();
            },

            getConversationById: function (id) {
                return ConversationService.getConversationById(id);
            },

            joinConversation: function(roomJid) {
                joinConversation(roomJid);
            },

            showInProgressCallBanner: showInProgressCallBanner,

            hideInProgressCallBanner: hideInProgressCallBanner,

            inProgressCallBannerHidden: inProgressCallBannerHidden,

            openConversation: function(nucleusId) {
                openConversation(nucleusId);
            },

            closeConversationById: function (id) {
                closeConversationById(id);
            },

            closeAllConversations: function () {
                closeAllConversations();
            },

            getActiveConversationId: function () {
                return activeConversationId;
            },

            markConversationReadById: function (id) {
                markConversationReadById(id);
            },

            getNumConversations: function () {
                return getNumConversations();
            },

            /**
             * @return {void}
             * @method
             */
            sendXmppMessage: function (to, messageText) {

                var strippedMsg = messageText.replace(/<[^>]*>/g, '');
                if (strippedMsg.length) {

                    Origin.xmpp.sendMessage(Origin.xmpp.nucleusIdToJid(to), messageText);

                    var msg = {
                        message: strippedMsg,
                        time: Date.now(),
                        from: Origin.user.userPid(),
                        type: 'message'
                    };

                    var conversationId = to;

                    // This is the first message in the conversation
                    if (typeof ConversationService.getConversationById(conversationId) === 'undefined') {

                        RosterDataFactory.getFriendInfo(msg.from).then(function (friend) {
                            ConversationService.setConversationById(conversationId, createConversation(conversationId, 'ONE_ON_ONE', friend.originId, [Number(msg.from)]));
                            updateVoiceSupportedRemotely(conversationId);
                            addMessageToConversation(conversationId, msg);
                            ChatWindowFactory.raiseConversation(conversationId.toString());
                        });

                    } else {
                        addMessageToConversation(conversationId, msg);
                    }
                }
            },

            /**
             * @return {void}
             * @method
             */
            sendMultiXmppMessage: function(to, messageText) {
                Origin.xmpp.sendMessage(to, messageText, 'groupchat');

                /*
                var msg = {
                    message: messageText,
                    time: Date.now(),
                    from: Origin.user.userPid(),
                    type: 'message'
                };

                var conversationId = to;

                // This is the first message in the conversation
                if (typeof ConversationService.getConversationById(conversationId) === 'undefined') {

                    RosterDataFactory.getFriendInfo(msg.from).then(function(friend) {
                        ConversationService.getConversationById(conversationId) = {
                            state: 'ONE_ON_ONE',
                            title: friend.originId,
                            participants: [ msg.from ],
                            events: [],
                            formattedEvents: []
                        };

                        addMessageToConversation(conversationId, msg);

                        ChatWindowFactory.raiseConversation(conversationId.toString());
                    });

                } else {
                    addMessageToConversation(conversationId, msg);
                }
                */
            },

            /**
             * @return {void}
             * @method
             */
            sendXmppTypingState: function(state, to) {
                Origin.xmpp.sendTypingState(state, to + '@integration.chat.dm.origin.com'); //TR Fix this
            }

        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ConversationFactorySingleton(UserDataFactory, SocialDataFactory, RosterDataFactory, AppCommFactory, ChatWindowFactory, SocialHubFactory, AuthFactory, VoiceFactory, EventsHelperFactory, ConversationService, ObservableFactory, GamesDataFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ConversationFactory', ConversationFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.ConversationFactory

     * @description
     *
     * ConversationFactory
     */
    angular.module('origin-components')
        .factory('ConversationFactory', ConversationFactorySingleton);
}());
