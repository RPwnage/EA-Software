(function() {
    'use strict';

    function ConversationFactory(UserDataFactory, SocialDataFactory, RosterDataFactory, AppCommFactory, ChatWindowFactory, SocialHubFactory) {

        var conversations= {},
            activeConversationId,
            gameActivityMap = {},
            offline = false,
            myEvents = new Origin.utils.Communicator();

        /**
         * 
         * @return New conversation object
         * @method
         */
        function createConversation(id, state, title, participants) {
            return { 
                id: id,
                state: state,
                title: title,
                participants: participants,
                unreadCount: 0,
                formattedEvents: [{ type: 'system', subtype: 'phishing' }]
            };
        }

        /**
         * 
         * @return The last event in the conversation, or null if none exists
         * @method
         */
        function getMostRecentConversationEvent(conversationId) {
            return conversations[conversationId].formattedEvents.length > 0 ? conversations[conversationId].formattedEvents[conversations[conversationId].formattedEvents.length-1] : null;
        }

        /**
            * 
            * @return latest timestamp or 0 if none found
            * @method
            */
        function findMostRecentTimestamp(conversationId) {                    
            // find latest timestamp event, return the time
            var latest = 0;
            if ((conversationId in conversations) && (conversations[conversationId].formattedEvents.length > 0)) {
                for (var x = conversations[conversationId].formattedEvents.length-1; x >= 0; --x) {
                    var event = conversations[conversationId].formattedEvents[x];
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
                console.log("linkify: " + urlText);
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
                                conversations[id].formattedEvents.push(monologueEvent);
                            } else {
                                // append message to existing monologue
                                mostRecentEvent.messages.push( htmlDecodedMessage );
                            }
                        } else {
                            // not from the same sender, so start a new monologue event
                            conversations[id].formattedEvents.push(monologueEvent);                    
                        }
                    } else {
                        // most recent event is not a monologue, so start a new monologue event
                        conversations[id].formattedEvents.push(monologueEvent);                    
                    }
                    
                } else {
                    // This is the first event, so add the new monologue event
                    conversations[id].formattedEvents.push(monologueEvent);                    
                }
            }
            
            // Add the message to the formatted list
            addFormattedMessage();

            //console.log('formattedEvents: ' + JSON.stringify(conversations[id].formattedEvents));

            //console.log('formattedEvents count: ' + conversations[id].formattedEvents.length);

            // Update the unread count
            conversations[id].unreadCount++;
            
            // Notify that a new message has been added
            myEvents.fire('addMessage', id.toString());

            // Notify that the conversation history has been updated
            myEvents.fire('historyUpdated', id.toString());
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
            if (typeof conversations[conversationId] === 'undefined') {

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
                        conversations[conversationId] = createConversation(conversationId, 'ONE_ON_ONE', '', [fromId]);
                        addMessageToConversation(conversationId, msg);
                        RosterDataFactory.getFriendInfo(fromId).then(function (friend) {
                            conversations[conversationId].title = friend.originId;
                            ChatWindowFactory.openConversation(conversationId.toString());
                        });                        
                    }
                }
                else {
                    conversations[conversationId] = createConversation(conversationId, 'ONE_ON_ONE', '', [fromId]);
                    addMessageToConversation(conversationId, msg);
                    RosterDataFactory.getFriendInfo(msg.from).then(function (friend) {
                        conversations[conversationId].title = friend.originId;
                        ChatWindowFactory.openConversation(conversationId.toString());
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

            if (xmppMsg.msgBody.length){
            
                fromId = Number(xmppMsg.jid.split('@')[0]);
                if (fromId === Origin.user.userPid()) {

                    // This is a carbon of a message from ourselves

                    if (xmppMsg.to.indexOf('gac_') === 0) {
                        // Carbon group chat message

                        // See if we have this conversation open already.
                        // If so, simply eat the message as we'll receive it normally as well
                        // If not, open the conversation and add the message

                        if (typeof conversations[xmppMsg.to] === 'undefined') {
                            var message = { 
                                message: xmppMsg.msgBody,
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
                            message: xmppMsg.msgBody,
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
                                message: xmppMsg.msgBody,
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
                            message: xmppMsg.msgBody,
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
                    conversations[id].formattedEvents.push(formattedPresence);
                }
            }

            addFormattedPresence();

            setTimeout(function () {
                myEvents.fire('historyUpdated', id.toString());
            }, 50);
      
            
            //console.log('formattedEvents: ' + JSON.stringify(conversations[id].formattedEvents));  
            
            //console.log('formattedEvents count: ' + conversations[id].formattedEvents.length);                        
        }
        
        /**
         * 
         * @return array of conversation IDs
         * @method
         */
        function getConversationIdArrayFromPresence(presence) {
            var IDs = [];

            for (var id in conversations) {
                var conv = conversations[id];
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
                    if (conversations[id].state === 'ONE_ON_ONE') {
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
                var participantsArrayIndex = conversations[conversationId].participants.indexOf(presence.from);
                if ((presence.subtype === 'participant_enter') && (participantsArrayIndex < 0)) {
                    conversations[conversationId].participants.push( presence.from );
                    myEvents.fire('updateParticipantList');
                }
                
                if ((presence.subtype === 'participant_exit') && (participantsArrayIndex >= 0)) {
                    conversations[conversationId].participants.splice(participantsArrayIndex, 1);
                    myEvents.fire('updateParticipantList');
                }
            }
            
            if (typeof conversations[conversationId]=== 'undefined') {
                // New conversation
                SocialDataFactory.getGroupName(conversationId).then(function(name) {
                    
                    conversations[conversationId] = createConversation(conversationId, 'MULTI_USER', name, [Number(presence.from)]);

                    if ((presence.subtype === 'participant_enter') || (presence.subtype === 'participant_exit')) {
                        if (presence.from !== Origin.user.userPid()) {
                            var formattedPresence = {
                                type: presence.type,
                                subtype: presence.subtype,
                                time: addTimestamp ? presence.time : undefined,
                                from: [ presence.from ]
                            };
                            conversations[conversationId].formattedEvents.push(formattedPresence);
                            myEvents.fire('historyUpdated', conversationId.toString());
                        }
                    }

                    updateParticipants();
                    ChatWindowFactory.raiseConversation(conversationId.toString());

                    //console.log('conversations['+conversationId+']: ' + JSON.stringify(conversations[conversationId]));  
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
                            conversations[conversationId].formattedEvents.push(formattedPresence);
                        }
                        myEvents.fire('historyUpdated', conversationId.toString());
                    }
                }

                updateParticipants();                    
                ChatWindowFactory.raiseConversation(conversationId.toString());
                //console.log('conversations['+conversationId+']: ' + JSON.stringify(conversations[conversationId]));  
            }
        }

        
        /**
         * 
         * @return {void}
         * @method
         */
        function onXmppPresenceChanged(xmppPresence) {
        
            //console.log('ConversationFactory: onXmppPresenceChanged presenceType:' + xmppPresence.presenceType);
            //console.log('ConversationFactory: onXmppPresenceChanged show:' + xmppPresence.show);
            //console.log('ConversationFactory: onXmppPresenceChanged gameActivity:' + JSON.stringify(xmppPresence.gameActivity));
            //console.log('ConversationFactory: onXmppPresenceChanged from:' + xmppPresence.from);
            //console.log('ConversationFactory: onXmppPresenceChanged jid:' + xmppPresence.jid);
            //console.log('ConversationFactory: onXmppPresenceChanged to:' + xmppPresence.to);

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
                if (xmppPresence.show === 'ONLINE' && typeof xmppPresence.presenceType === 'undefined') {
                    subtype = 'participant_enter';
                } else if (xmppPresence.show === 'ONLINE' && xmppPresence.presenceType === 'UNAVAILABLE') {
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
                    // See if this is a user starting or stopping a game
                    if (xmppPresence.gameActivity && xmppPresence.gameActivity.title !== undefined && xmppPresence.gameActivity.title !== '') {
                        // User started playing a game
                        gameActivityMap[nucleusId] = xmppPresence.gameActivity;
                        gameRelated = true;
                    }
                    else if ((typeof gameActivityMap[nucleusId] !== 'undefined') && gameActivityMap[nucleusId].title !== undefined && gameActivityMap[nucleusId].title !== '') {
                        // User stopped playing a game
                        delete gameActivityMap[nucleusId];
                        gameRelated = true;
                    }

                    if (!gameRelated) {
                        addPresenceEventOneToOne({
                            type: 'system',
                            subtype: 'presence',
                            time: eventTime,
                            from: nucleusId,
                            presence: ((xmppPresence.presenceType === 'UNAVAILABLE') ? 'offline' : xmppPresence.show.toLowerCase())
                        });
                    }
                }
            }
            
        }

        function onClientOnlineStateChanged(online) {
            var id, conv;

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

            if (online) {
                // Remove previous "you are offline" system message from all conversations
                for (id in conversations) {
                    conv = conversations[id];
                    for (var x = conv.formattedEvents.length - 1; x >= 0; --x) {
                        var event = conv.formattedEvents[x];
                        if (event.subtype === 'offline_mode') {
                            conv.formattedEvents.splice(x, 1);
                            myEvents.fire('historyUpdated', id.toString());
                            break; // assume there's only one at the end
                        }
                    }
                }
            }
            else {
                // Add a "you are offline" system message to all conversations
                var eventTime = Date.now();

                for (id in conversations) {
                    conv = conversations[id];
                    var addTimestamp = shouldAddTimestamp(id, eventTime);
                    var formattedEvent = {
                        type: 'system',
                        subtype: 'offline_mode',
                        time: addTimestamp ? eventTime : undefined
                    };
                    conv.formattedEvents.push(formattedEvent);
                    myEvents.fire('historyUpdated', id.toString());
                }
            }

        }

        function handleRemovedFriend(nucleusId) {
            // If we unfriend a user with an open conversation,
            // we never want the last event to be a presence event.
            var conv = conversations[nucleusId];
            if (!!conv) {
                var event = getMostRecentConversationEvent(conv.id);
                if (event && event.type === 'system' && event.subtype === 'presence') {
                    conv.formattedEvents.pop();
                    myEvents.fire('historyUpdated', conv.id.toString());
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

        function closeConversationById(id) {
            delete conversations[id];
            
            if (id.indexOf('gac_')===0) {
                Origin.xmpp.leaveRoom(id, Origin.user.originId());
            }
            
            myEvents.fire('conversationClosed', id.toString());
        }

        function closeAllConversations() {
            for (var id in conversations) {
                closeConversationById(id);
            }
        }

        function markConversationReadById(id) {
            conversations[id].unreadCount = 0;
            myEvents.fire('conversationRead', id);
        }
        
        function init() {
            //console.log('ConversationFactory: init()');

            // listen for incoming Xmpp messages
            Origin.events.on(Origin.events.XMPP_INCOMINGMSG, onXmppIncomingMessage);

            // listen for XMPP presence changes
            Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, onXmppPresenceChanged);
            
            //listen for connection change (for embedded)
            Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);

            // listen for join Conversation events
            SocialDataFactory.events.on('joinConversation', joinConversation);

            // Listen for unfriending
            RosterDataFactory.events.on('removedFriend', handleRemovedFriend);
        }
        
        return {

            init: init,
            events: myEvents,

            getConversations: function() {
                return conversations;
            },

            getConversationById: function (id) {
                return conversations[id];
            },
            
            joinConversation: function(roomJid) {
                joinConversation(roomJid);
            },
            
            openConversation: function(nucleusId) {
                var conversationId = nucleusId;
                if (typeof conversations[conversationId] === 'undefined') {
                    RosterDataFactory.getFriendInfo(nucleusId).then(function(friend) {
                        conversations[conversationId] = createConversation(conversationId, 'ONE_ON_ONE', friend.originId, [Number(nucleusId)]);

                        // Add an initial "offline" message
                        if (friend.presence.show === 'UNAVAILABLE') {
                            addPresenceToConversation(conversationId, {
                                type: 'system',
                                subtype: 'presence',
                                time: Date.now(),
                                from: nucleusId,
                                presence: 'offline'
                            });
                        }

                        // always raise the conversation
                        ChatWindowFactory.raiseConversation(conversationId.toString()); 
                        if (SocialHubFactory.isWindowPoppedOut() && !ChatWindowFactory.isWindowPoppedOut()) {
                            ChatWindowFactory.popOutWindow();
                        }  
                    });
                }
                else {
                     // always raise the conversation
                    ChatWindowFactory.raiseConversation(conversationId.toString());  
                    if (SocialHubFactory.isWindowPoppedOut() && !ChatWindowFactory.isWindowPoppedOut()) {
                        ChatWindowFactory.popOutWindow();
                    } 
                }
            },

            closeConversationById: function (id) {
                closeConversationById(id);
            },

            closeAllConversations: function () {
                closeAllConversations();
            },

            setActiveConversationId: function (id) {
                if (id !== activeConversationId) {
                    activeConversationId = id;
                    myEvents.fire('activeConversationIdChanged', activeConversationId.toString());
                }
            },

            getActiveConversationId: function () {
                return activeConversationId;
            },

            markConversationReadById: function (id) {
                markConversationReadById(id);
            },
            
            getNumConversations: function () {
                return Object.keys(conversations).length;
            },


            /**
             * @return {void}
             * @method
             */
            sendXmppMessage: function(to, messageText) {
                Origin.xmpp.sendMessage(to + '@integration.chat.dm.origin.com', messageText); //TR Fix this

                var msg = { 
                    message: messageText,
                    time: Date.now(),
                    from: Origin.user.userPid(),
                    type: 'message'
                };                                
                
                var conversationId = to;

                // This is the first message in the conversation
                if (typeof conversations[conversationId] === 'undefined') {
                
                    RosterDataFactory.getFriendInfo(msg.from).then(function(friend) {
                        conversations[conversationId] = createConversation(conversationId, 'ONE_ON_ONE', friend.originId, [Number(msg.from)]);
                        addMessageToConversation(conversationId, msg);
                        ChatWindowFactory.raiseConversation(conversationId.toString());
                    });                        
                
                } else {
                    addMessageToConversation(conversationId, msg);
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
                if (typeof conversations[conversationId] === 'undefined') {
                
                    RosterDataFactory.getFriendInfo(msg.from).then(function(friend) {
                        conversations[conversationId] = { 
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
    function ConversationFactorySingleton(UserDataFactory, SocialDataFactory, RosterDataFactory, AppCommFactory, ChatWindowFactory, SocialHubFactory, SingletonRegistryFactory) {
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