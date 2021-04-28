/*global clearTimeout, clientNavigation, console, dateFormat, document, jQuery, Origin, originChat,
         originSocial, productQuery, telemetryClient, tr, window */
(function ($) {
    "use strict";

    var conversationId, conversation, oneToOnePartner, updateHistoryWith,
    conditionalEventTimestamp, handleMessageEvent, handleMessageQueue,
    handleChatStateNotificationEvent, handlePresenceEvent,
    handleSubscriptionStateEvent, handleMultiplayerInviteEvent, handleRoomDestroyedEvent,
    handleConversationEvent, threadId = null, timestampGroup, monologueList,
    $textInput, $history, dialogueListItem = null, $monologue, lastMessageUserId,
    handleHistoryUserEvent, handleChannelUserEvent, handleConnectionChange, $timestampStyle,
    updateTimestampStyle, notifyParentWindow, partnerNameChanged,
    handleUserActionLink, $takeLastEventIfMatches, $initialPresence,
    $monologueStyle, setMonologueMaxWidth, setConversationTitle, setGroupTitle,
    handleEventWith, updateInputField, conditionalNonFriendMessage,
    handleCompressingEventWith, scrollHistoryToBottom,
    previousConversations, lastConversation = null, suppressEnterEvents = false,
    lastSetMaxWidth = null, wasConversationActive, wasConnectionEstablished,
    handleCallIncoming, handleVoipRemoteCalling, handleVoipLocalCalling, handleVoipInputDeviceChange,
    handleVoipCallStarted, handleVoipCallEnded, handleVoipNoAnswer, handleVoipMissedCall, handleVoipInactivity,
    composing = false, composingTimeout, $chatState = null, $participantList,
    $participantItemForUserId, onUpdateSettings, pushToTalk = false, talking = false,
    emptyParticipantList, createParticipantList, addParticipantToList, addParticipant, removeParticipant,
    maxHistoryItems = 100, showChannelUserMenu, updateVoipToolTip, updateParticipantCount, messageQueue = [],
    onVisibilityChanged, setParticipantListHeight, lastHistoryListHeight = null, mainContentTop,
    $footer, $participantFooter, setHistoryListHeight, lastParticipantListHeight = null, mostRecentChannelId = '',
    microphoneWasDisconnected = false, audioInputDevice = "";

    // Set up global error code enum
    window.ChatroomEnteredStatus = {
        //! User successfully entered the room
        CHATROOM_ENTERED_SUCCESS: 0,
        //! The nickname conflicted with somebody already in the room
        CHATROOM_ENTERED_FAILURE_NICKNAME_CONFLICT: 1,
        //! A password is required to enter the room
        CHATROOM_ENTERED_FAILURE_PASSWORD_REQUIRED: 2,
        //! The specified password was incorrect
        CHATROOM_ENTERED_FAILURE_PASSWORD_INCORRECT: 3,
        //! The user is not a member of a member-only room
        CHATROOM_ENTERED_FAILURE_NOT_A_MEMBER: 4,
        //! The user cannot enter because the user has been banned
        CHATROOM_ENTERED_FAILURE_MEMBER_BANNED: 5,
        //! The room has the maximum number of users already
        CHATROOM_ENTERED_FAILURE_MAX_USERS: 6,
        //! The room has been locked by an administrator
        CHATROOM_ENTERED_FAILURE_ROOM_LOCKED: 7,
        //! Someone in the room has blocked you
        CHATROOM_ENTERED_FAILURE_MEMBER_BLOCKED: 8,
        //! You have blocked someone in the room
        CHATROOM_ENTERED_FAILURE_MEMBER_BLOCKING: 9,
        //! Client is old. User must upgrade to a more recent version for
        // hangouts to work.
        CHATROOM_ENTERED_FAILURE_OUTDATED_CLIENT: 10,
        // Group or room could not be found on the server
        CHATROOM_ENTERED_FAILURE_GROUP_NOT_FOUND: 11,
        //! Some other reason
        CHATROOM_ENTERED_FAILURE_UNSPECIFIED: 2000
    };

    // Make a style element for hiding/showing timestamps
    $timestampStyle = $('<style type="text/css">').appendTo($('head'));
    $monologueStyle = $('<style text="text/css">').appendTo($('head'));

    // Get some important elements
    $textInput = $('#text-input');
    $history = $('ol#history');
    $participantList = $('ol#participant-list');
    $footer = $("#text-input-area");
    $participantFooter = $("#participant-section-footer");

    // Get main-content offset value for height calculations
    if ($("#main-content").length > 0) {
        mainContentTop = $("#main-content").offset().top;
    } else {
        mainContentTop = $("#history-section").offset().top;
    }

    // Look up our conversation
    conversationId = $.getQueryString("id");
    conversation = originChat.getConversationById(conversationId);

    if (conversation === null) {
        console.warn("Unable to look up conversation: " + conversationId);
        return;
    }

    wasConversationActive = conversation.active;
    wasConnectionEstablished = originSocial.connection.established;

    // End the conversation when we close
    conversation.finishOnClose = true;

    // Track our current timestamp group
    timestampGroup = null;

    $participantItemForUserId = function (id) {
        return $participantList.find('li.group-participant-list-item[data-user-id="' + id + '"]');
    };

    updateVoipToolTip = function (remoteUser) {

        var $listItem, $iconVoip, self, muted, disabled;

        $listItem = $participantItemForUserId(remoteUser.id);
        $iconVoip = $listItem.find(".icon.voip");

        self = remoteUser.id == originSocial.currentUser.id ? true : false;
        muted = self ? (conversation.voiceCallState.indexOf("MUTED") !== -1) : (conversation.remoteUserVoiceCallState(remoteUser).indexOf("REMOTE_VOICE_LOCAL_MUTE") !== -1);
        disabled = $listItem.hasClass('remote-mute') || $listItem.hasClass('admin-mute');
        if (self && pushToTalk && !talking) {
            disabled = true;
        }

        if (disabled) {
            $iconVoip.removeAttr('title');
        }
        else {
            if (self) {
                if (muted) {
                    $iconVoip.attr('title', tr('Ebisu_client_unmute_your_microphone'));
                }
                else {
                    $iconVoip.attr('title', tr('ebisu_client_mute_your_microphone'));
                }
            }
            else {
                if (muted) {
                    $iconVoip.attr('title', tr('ebisu_client_unmute_user').replace('%1', remoteUser.nickname));
                }
                else {
                    $iconVoip.attr('title', tr('ebisu_client_mute_user').replace('%1', remoteUser.nickname));
                }
            }
        }
    };

    updateParticipantCount = function () {
        var numParticipants, $countLabel;

        $countLabel = $('footer#participant-section-footer #participant-count');
        numParticipants = $participantList.find('li.group-participant-list-item').length;
        if (numParticipants === 0) {
            $countLabel.text(tr('ebisu_client_not_in_chat'));
        }
        else {
            $countLabel.text(tr('ebisu_client_in_chat').replace('%1', numParticipants));
        }

        // Kill the "invite members" link if we are no longer alone in the room

        /*      
        
        design isn't sure if they want this or not

        if (conversation.participants.length > 0)
        {
            $history.find($('li#empty-room-notice')).remove();
        }

        */
    };

    addParticipantToList = function (remoteUser, $listItem, disableSort) {
        var newSortKey, inserted;

        if (!originSocial.connection.established) {
            return;
        }

        if (disableSort) {
            // Pre-sorted
            $listItem.appendTo($participantList);
        }
        else {
            // Find the proper insertion point

            newSortKey = $listItem.attr('data-sort-key');
            inserted = false;

            // Find our correct sort position
            $.each($participantList.children(), function () {
                var $existingListItem = $(this);

                if (newSortKey < $existingListItem.attr('data-sort-key')) {
                    // We found our insertion place
                    $listItem.insertBefore($existingListItem);
                    inserted = true;
                    return false;
                }
            });

            if (!inserted) {
                // Add to the end
                $participantList.append($listItem);
            }
        }
    };

    addParticipant = function (remoteUser, disableSort) {
        var remoteUserChanged,
            disconnectAll,
            $listItem,
            $avatar,
            $presenceIndicator,
            $nickname,
            $subtitle,
            $iconVoip,
            $iconBroadcasting,
            $iconJoinable,
            self,
            voipCallState = "";

        // Make sure they're not already in the list
        if ($participantList.find('li[data-user-id="' + remoteUser.id + '"]').length) {
            return false;
        }

        self = remoteUser.id == originSocial.currentUser.id ? true : false;

        // Create a new list item for this user and add it to the list
        if (self) {
            $listItem = $(
                '<li class="group-participant-list-item self">' +
                    '<div class="otk-presence-indicator"></div>' +
                    '<img class="otk-user-avatar">' +
                    '<span class="labels">' +
                        '<span class="nickname"></span>' +
                        '<span class="subtitle otk-hyperlink"></span>' +
                    '</span>' +
                    '<span class="icons">' +
                        '<span id="icon_voip" class="icon voip"></span>' +
                    '</span>' +
                    '<button class="otk-hover-context-menu-button"></button>' +
                '</li>'
                );
        }
        else {
            $listItem = $(
                '<li class="group-participant-list-item">' +
                    '<div class="otk-presence-indicator"></div>' +
                    '<img class="otk-user-avatar">' +
                    '<span class="labels">' +
                        '<span class="nickname"></span>' +
                        '<span class="subtitle otk-hyperlink"></span>' +
                    '</span>' +
                    '<span class="icons">' +
                        '<span class="icon broadcasting" id="icon_broadcast"></span>' +
                        '<span class="icon joinable" id="icon_joinable"></span>' +
                        '<span class="icon voip" id="icon_voip"></span>' +
                    '</span>' +
                    '<button class="otk-hover-context-menu-button"></button>' +
                '</li>'
                );
        }

        $listItem.attr('data-user-id', remoteUser.id);
        $listItem.attr('data-sort-key', Origin.utilities.sortKeyForUser(remoteUser));

        // Add it to the proper place in the list
        addParticipantToList(remoteUser, $listItem, disableSort);

        // Cache some of the elements we will be referencing a lot
        $avatar = $listItem.find(".otk-user-avatar");
        $presenceIndicator = $listItem.find(".otk-presence-indicator");
        $nickname = $listItem.find(".nickname");
        $subtitle = $listItem.find(".subtitle");

        // Set the user's initial voip state
        if (!self) {
            voipCallState = conversation.remoteUserVoiceCallState(remoteUser);
            if (voipCallState.indexOf("REMOTE_VOICE_NONE") !== -1) {
            }
            if (voipCallState.indexOf("REMOTE_VOICE_ACTIVE") !== -1) {
                $listItem.addClass("voip");
            }
            if (voipCallState.indexOf("REMOTE_VOICE_LOCAL_MUTE") !== -1) {
                $listItem.addClass("local-mute");
            }
            if (voipCallState.indexOf("REMOTE_VOICE_REMOTE_MUTE") !== -1) {
                $listItem.addClass("remote-mute");
            }
        }

        // Set the dynamic voip icon tooltip
        $iconVoip = $listItem.find(".icon.voip");
        updateVoipToolTip(remoteUser);

        // Set the static icon tooltips
        $iconBroadcasting = $listItem.find(".icon.broadcasting");
        $iconBroadcasting.attr('title', tr("ebisu_client_watch_broadcast"));
        $iconJoinable = $listItem.find(".icon.joinable");
        $iconJoinable.attr('title', tr("ebisu_client_join_game"));

        remoteUserChanged = function () {
            var newSortKey, oldSortKey, isEmbargoed;

            isEmbargoed = remoteUser.playingGame && (remoteUser.playingGame.gameTitle === tr('ebisu_client_notranslate_embargomode_title'));

            // Username
            $nickname.text(remoteUser.nickname);

            // Blocked user
            if (remoteUser.blocked) {
                $subtitle.text(tr('ebisu_client_blocked')).addClass('blocked');
                $subtitle.removeClass("otk-hyperlink");
            } else {
                // Game name
                $subtitle.text(remoteUser.playingGame ? remoteUser.playingGame.gameTitle : "");
                $subtitle.toggleClass("otk-hyperlink", remoteUser.playingGame && remoteUser.playingGame.purchasable && !isEmbargoed);
            }

            // Presence indicator
            $presenceIndicator.attr("data-presence", Origin.utilities.presenceStringForUser(remoteUser));

            // Avatar
            if (remoteUser.avatarUrl) {
                $avatar.attr("src", remoteUser.avatarUrl);
            }
            else {
                $avatar.removeAttr("src");
            }

            // Hide & show broadcasting and joinable icons.
            // Voip icons are updated in the conversation event handler.
            if (remoteUser.playingGame && remoteUser.playingGame.joinable) {
                $listItem.addClass("joinable");
            }
            else {
                $listItem.removeClass("joinable");
            }
            if (remoteUser.playingGame && remoteUser.playingGame.broadcastUrl) {
                $listItem.addClass("broadcasting");
            }
            else {
                $listItem.removeClass("broadcasting");
            }

            // See if we need to change the sort position
            newSortKey = Origin.utilities.sortKeyForUser(remoteUser);
            oldSortKey = $listItem.attr('data-sort-key');

            if (newSortKey !== oldSortKey) {
                // Remove it from the previous location and put it into the new sort location
                $listItem.detach();
                $listItem.attr('data-sort-key', newSortKey);
                addParticipantToList(remoteUser, $listItem, false);
            }
        };

        // Listen for changes to the remote user
        remoteUser.presenceChanged.connect(remoteUserChanged);
        remoteUser.nameChanged.connect(remoteUserChanged);
        remoteUser.avatarChanged.connect(remoteUserChanged);
        if (!!remoteUser.blockChanged) {
            remoteUser.blockChanged.connect(remoteUserChanged);
        }

        disconnectAll = function () {
            // Kill our signals
            remoteUser.presenceChanged.disconnect(remoteUserChanged);
            remoteUser.nameChanged.disconnect(remoteUserChanged);
            remoteUser.avatarChanged.disconnect(remoteUserChanged);
            if (!!remoteUser.blockChanged) {
                remoteUser.blockChanged.disconnect(remoteUserChanged);
            }
        };

        // Set a callback that can disconnect our signals
        $listItem.data("disconnectAll", disconnectAll);

        // Set up our initial presence, etc
        remoteUserChanged();

        updateParticipantCount();

        return true;
    };

    removeParticipant = function (remoteUser) {
        var $listItem = $participantItemForUserId(remoteUser.id);

        // Disconnect this list item's signals
        $listItem.data("disconnectAll")();

        $listItem.remove();

        updateParticipantCount();
    };

    emptyParticipantList = function () {
        // Disconnect and remove every item in the list
        $.each($participantList.children(), function () {
            var $listItem = $(this);
            $listItem.data("disconnectAll")();
            $listItem.remove();
        });

        updateParticipantCount();
    };

    createParticipantList = function () {
        if ($participantList.length) {
            emptyParticipantList();

            // Pre-sort the list and add ourselves and everyone else currently in the group chat
            conversation.participants.concat([originSocial.currentUser]).map(function (c) {
                return { key: Origin.utilities.sortKeyForUser(c), user: c };
            })
            // Sort by the keys
            .sort(function (a, b) {
                if (a.key > b.key) {
                    return 1;
                }

                if (a.key < b.key) {
                    return -1;
                }

                return 0;
            })
            // Add the contacts
            .forEach(function (c) {
                // Add the participant but disable sorting since we've already pre-sorted!
                addParticipant(c.user, true);
            });
        }
    };

    if (conversation.type === 'ONE_TO_ONE') {
        oneToOnePartner = conversation.participants[0];
    }
    else {
        // Create our initial list of participants
        createParticipantList();
    }

    scrollHistoryToBottom = function () {
        $history[0].scrollTop = $history[0].scrollHeight;
    };

    updateHistoryWith = function (updater) {

        var numHistoryItems;

        if (typeof updater === 'function') {
            // Assume this is a callback function
            updater();
        }
        else {

            if ($chatState) {
                $chatState.remove();
            }

            // Assume this is a jQuery collection
            $history.append(updater);

            if ($chatState) {
                $chatState.appendTo($history);
            }

            // If we're adding a new element we have to kill the dialogue to make
            // sure we don't start appending messages abovet this element
            dialogueListItem = null;
        }

        // Keep our history at a reasonable length
        // We may want to just visibility:hidden on these items and add a
        // "show previous items" link, but for now I'm just going to be removing them.
        numHistoryItems = $('ol#history > li').length;
        if (numHistoryItems > maxHistoryItems) {
            $('ol#history > li').slice(0, numHistoryItems - maxHistoryItems).remove();
        }

        if (!Origin.views.isManuallyScrolled()) {
            scrollHistoryToBottom();
        }
    };

    notifyParentWindow = function () {
        if (window.parent && window.parent.postMessage) {
            // Notify our container window that something interesting happened
            window.parent.postMessage({
                'type': 'conversationNotification',
                'conversationId': conversationId
            }, '*');
        }
    };

    setMonologueMaxWidth = function () {
        var maxWidth = $('body').width() - 150;

        if (conversation.type === 'ONE_TO_ONE') {
            // Allow 10px of fuzz to prevent style recalc storms when resizing
            if (!lastSetMaxWidth || (Math.abs(maxWidth - lastSetMaxWidth) > 10)) {
                // Constrain our width so we wrap properly
                $monologueStyle.text('ol#history > li.dialogue > ol.monologue { max-width: ' + maxWidth + 'px; }');

                lastSetMaxWidth = maxWidth;
            }
        }
    };

    setHistoryListHeight = function () {
        var height = $footer.offset().top - mainContentTop - 1; // 1px border width

        if (!lastHistoryListHeight || (height !== lastHistoryListHeight)) {
            lastHistoryListHeight = height;
            $history.height(height);
        }

    }

    
    setParticipantListHeight = function () {
        if ($participantFooter.length > 0) {
            var height = $participantFooter.offset().top - mainContentTop - 1; // 1px border width

            if (!lastParticipantListHeight || (height !== lastParticipantListHeight)) {
                lastParticipantListHeight = height;
                $participantList.height(height);
            }
        }
    }

    // Inserts a timestamp in the history
    conditionalEventTimestamp = function (evt) {
        var formattedTime;

        if (conversation.type === 'ONE_TO_ONE') {
            // Is this part of our current timestamp group?
            if (timestampGroup && timestampGroup.continueWith(evt)) {
                // Yes, nothing to do
                return;
            }

            // Append the time to the history
            formattedTime = dateFormat.formatTime(evt.time, false);
            $('<li class="time">').text(formattedTime).appendTo($history);

            // Kill the previous dialogueListItem element or else messages can appear above
            // the timestamp we just inserted
            dialogueListItem = null;

            timestampGroup = new Origin.utilities.TimestampGroup(evt);
        }
    };

    conversation.typeChanged.connect(function () {
        // Not sure what we need to do here
    });

    // Listen for groups being changed. This only updates the tab title.
    conversation.updateGroupName.connect(function (groupGuid, groupName) {
        if (window.parent && window.parent.postMessage) {
            // Notify our container window that something interesting happened
            window.parent.postMessage({
                'type': 'updateConversationName',
                'conversationId': conversationId,
                'name': groupName
            }, '*');
        }

        setGroupTitle();
    });

    // function that takes message event, and puts it into the queue
    handleMessageEvent = function (messageEvent) {
        messageQueue.push(messageEvent);
    }

    // Handle a message sent or received in the conversation
    handleMessageQueue = function () {
        var queue = messageQueue.splice(0),        // make local copy of message queue
            historyFragment = document.createDocumentFragment(),
            updateParentWindow = false;

        // clear out message queue
        messageQueue.length = 0;

        $.each(queue, function (i, messageEvent) {
            var newMonologue, sender, avatarUrl, $avatar, $nickname, nicknameSpan, timestampSpan;

            sender = messageEvent.from;

            if ($chatState && sender.id !== originSocial.currentUser.id) {
                // Remove the senders info from the chat state
                $chatState.find('img.avatar[data-user-id="' + sender.id + '"]').remove();
                $chatState.find('li.composing[data-user-id="' + sender.id + '"]').remove();
                $chatState.find('li.idle[data-user-id="' + sender.id + '"]').remove();

                if ($chatState.find('img.avatar').length === 0) {
                    $chatState.remove();
                    $chatState = null;
                }
            }

            // Assume all new message are replying to/following up this one
            threadId = messageEvent.threadId;

            conditionalEventTimestamp(messageEvent);

            if (conversation.type === "MULTI_USER") {
                // In group chats we don't really use the concept of dialogues but
                // we don't want to change the HTML structure from the 1:1 conversations
                // or our styles and code will have to be conditional. Just use
                // a dialogue as a monologue wrapper for group chats.
                if (lastMessageUserId !== messageEvent.from.id) {
                    dialogueListItem = null;
                }

                // Is this part of our current timestamp group?
                if (timestampGroup && timestampGroup.continueWith(messageEvent)) {
                    // Yes, nothing to do
                }
                else {
                    // Too much time has elapsed since the last message. Start a new dialog with a new timestamp
                    dialogueListItem = null;
                    timestampGroup = new Origin.utilities.TimestampGroup(messageEvent);
                }
            }

            // Make sure we have an open dialogueListItem element
            if (dialogueListItem === null) {
                // Nope, start a new dialogue
                dialogueListItem = document.createElement("li");
                dialogueListItem.className = "dialogue";
                if (messageEvent.historic)
                {
                    dialogueListItem.className += " historic";
                }

                // Append the new dialogue                
                historyFragment.appendChild(dialogueListItem);

                // A new dialogue implies a new monologue
                newMonologue = true;
            }
            else {
                // Start a new monologue within an existing dialogue if the speakers
                // change
                newMonologue = (lastMessageUserId !== messageEvent.from.id);
            }

            if (newMonologue) {
                monologueList = document.createElement("ol");
                monologueList.className = "monologue";

                if (conversation.type === 'ONE_TO_ONE') {
                    // Append the monologue to the dialogue
                    dialogueListItem.appendChild(monologueList);

                    // Add the avatar if this is the initial message
                    avatarUrl = sender.avatarUrl;
                    if (avatarUrl !== null) {
                        $avatar = $('<img class="avatar">').attr('src', avatarUrl);
                        $avatar.attr('data-user-id', sender.id);
                        monologueList.appendChild($avatar.get(0));
                    }
                }
                else {
                    // Show usernames
                    nicknameSpan = document.createElement("span");
                    nicknameSpan.className = "nickname";
                    nicknameSpan.appendChild(document.createTextNode(sender.nickname));
                    nicknameSpan.setAttribute('data-user-id', sender.id);

                    dialogueListItem.appendChild(nicknameSpan);


                    // Append the monologue to the dialogue
                    dialogueListItem.appendChild(monologueList);

                    // Show timestamps for every dialogue
                    timestampSpan = document.createElement("span");
                    timestampSpan.className = "timestamp";
                    timestampSpan.appendChild(document.createTextNode(dateFormat.formatTime(messageEvent.time, false)));
                    dialogueListItem.appendChild(timestampSpan);
                }

                if (sender.id === originSocial.currentUser.id) {
                    // Messages from me have a different style
                    monologueList.className += " self ";
                    if (conversation.type === "MULTI_USER") {
                        dialogueListItem.className += " self ";
                    }
                }
            }

            lastMessageUserId = messageEvent.from.id;

            updateHistoryWith(function () {
                if ($chatState) {
                    $chatState.remove();
                }

                monologueList.appendChild(Origin.views.monologueEntry(messageEvent));

                if ($chatState) {
                    $chatState.appendTo($history);
                }
            });

            if (!messageEvent.historic &&
            (messageEvent.from.id !== originSocial.currentUser.id)) {
                updateParentWindow = true;
            }

        });

        var scrollToBottom = !Origin.views.isManuallyScrolled();

        $history.get(0).appendChild(historyFragment);

        if (scrollToBottom) scrollHistoryToBottom();

        if (updateParentWindow) notifyParentWindow();
    };

    setInterval(handleMessageQueue, 100);    // process the message queue 10 times per second

    // We need to watch for when the microphone is disconected/connected
    if (originVoice && originVoice.supported) {
        var devices = originVoice.audioInputDevices;
        audioInputDevice = originVoice.selectedAudioInputDevice;
        if (devices.length === 0) {
            microphoneWasDisconnected = true;
        }
        originVoice.deviceRemoved.connect(Origin.utilities.reverse_throttle( function() {
            var devices = originVoice.audioInputDevices;
            console.log("DEVICE REMOVED - LENGTH = " + devices.length);
            if (devices.length === 0) {
                console.log("PATH 1");
                // If there are no devices connected now then obviously the microphone was unplugged
                microphoneWasDisconnected = true;
                handleVoipInputDeviceChange(tr("ebisu_client_unplugged"));
            }
            else {
                // Otherwise see if the device that was removed is the one we were previously using
                console.log("PATH 2");
                var inputDevice = originVoice.selectedAudioInputDevice;
                if (inputDevice !== audioInputDevice) {
                    // The input device has changed as a result of unplugging the current device
                    audioInputDevice = inputDevice;
                    handleVoipInputDeviceChange(inputDevice);
                }
            }
        }, 1000));
        originVoice.deviceAdded.connect(Origin.utilities.reverse_throttle( function() {
            var inputDevice = originVoice.selectedAudioInputDevice;
            if (microphoneWasDisconnected) {
                // Previously there was no microphone, now there is
                microphoneWasDisconnected = false;
                handleVoipInputDeviceChange(inputDevice);
            }
            else if (inputDevice !== audioInputDevice) {
                // The input device has changed as a result of plugging in a new device
                audioInputDevice = inputDevice;
                handleVoipInputDeviceChange(inputDevice);
            }
        }, 1000));
    }

    handleChatStateNotificationEvent = function (chatStateEvent) {
        var $stateMonologue, $notification, $avatar, sender, avatarUrl, numAvatars;

        if (conversation.type === 'ONE_TO_ONE') {
            sender = chatStateEvent.from;

            if ($chatState && sender.id !== originSocial.currentUser.id) {
                // Remove the senders info from the chat state
                $chatState.find('img.avatar[data-user-id="' + sender.id + '"]').remove();
                $chatState.find('li.composing[data-user-id="' + sender.id + '"]').remove();
                $chatState.find('li.idle[data-user-id="' + sender.id + '"]').remove();

                if ($chatState.find('img.avatar').length === 0) {
                    $chatState.remove();
                    $chatState = null;
                }
            }

            // If we have a state message of active or gone then we just cleared our message
            if (chatStateEvent.state === 'ACTIVE' || chatStateEvent.state === 'GONE') {
                return;
            }

            if ($chatState === null) {
                $chatState = $('<li class="dialogue">');
                $stateMonologue = $('<ol class="monologue">');
            }
            else {
                $stateMonologue = $chatState.find("ol.monologue");
            }

            // Add the notification
            $notification = Origin.views.$chatStateNotification(chatStateEvent);
            $notification.attr('data-user-id', sender.id);
            $notification.appendTo($stateMonologue);

            avatarUrl = sender.avatarUrl;
            if (avatarUrl !== null) {
                numAvatars = $chatState.find('img.avatar').length;
                $avatar = $('<img class="avatar">').attr('src', avatarUrl);
                $avatar.attr('data-user-id', sender.id);

                if (numAvatars > 0) {
                    $avatar.attr('style', 'margin-left: ' + (-50 - numAvatars * 4) + 'px; margin-top: ' + (-23 - numAvatars * 4) + 'px;');
                }

                $avatar.appendTo($stateMonologue);
            }

            $stateMonologue.appendTo($chatState);

            updateHistoryWith(function () {
                $chatState.appendTo($history);
            });
        }
    };

    $takeLastEventIfMatches = function (selector) {
        var $existingPresence, $existingPresenceTimestamp;

        $existingPresence = $history.children(selector + ':last-child');

        if (conversation.type === 'ONE_TO_ONE') {
            if ($existingPresence.length !== 0) {
                $existingPresenceTimestamp = $existingPresence.prev('li.time');

                if ($existingPresenceTimestamp.length !== 0) {
                    // Kill our current timestamp
                    $existingPresenceTimestamp.remove();
                    // Force us to create a new one
                    timestampGroup = null;
                }
            }
        }

        // Remove the presence event itself
        $existingPresence.remove();

        return $existingPresence;
    };

    // Listen for presence changes
    handlePresenceEvent = function (presenceEvent) {

        var userId, $newPresence = null, broadcasting, wasBroadcasting, joinable;

        if (presenceEvent.availability === null) {
            // We don't know our presence - just ignore it
            return;
        }

        broadcasting = presenceEvent.playingGame &&
                   presenceEvent.playingGame.broadcastUrl;

        wasBroadcasting = presenceEvent.previousPlayingGame &&
                      presenceEvent.previousPlayingGame.broadcastUrl;

        joinable = presenceEvent.playingGame &&
                   presenceEvent.playingGame.joinable;

        userId = presenceEvent.user.id;

        if (conversation.type === 'ONE_TO_ONE') {
            // Determine which presence to show
            if (presenceEvent.user.playingGame) {
                // Remove any previous game-related notifications
                $history.find('li.game').remove();

                $newPresence = Origin.views.$gamePresenceMessage(presenceEvent.user, presenceEvent.playingGame);
            }
            else if (broadcasting && !wasBroadcasting) {
                // Remove any previous game-related notifications
                $history.find('li.game').remove();

                $newPresence = Origin.views.$broadcastEndedMessage(presenceEvent.user);
            }
            else {
                if (presenceEvent.availability === 'UNAVAILABLE') {
                    $newPresence = Origin.views.$offlineRecipientNotice(presenceEvent.user);
                    Origin.views.disableToolbarButtons();

                    // If a user has gone offline, clear any composing flags for this user
                    if ($chatState && userId !== originSocial.currentUser.id) {
                        // Remove the senders info from the chat state
                        $chatState.find('img.avatar[data-user-id="' + userId + '"]').remove();
                        $chatState.find('li.composing[data-user-id="' + userId + '"]').remove();
                        $chatState.find('li.idle[data-user-id="' + userId + '"]').remove();
                    }

                    if ($chatState && $chatState.find('img.avatar').length === 0) {
                        $chatState.remove();
                        $chatState = null;
                    }
                }
                else {
                    Origin.views.enableToolbarButtons();
                    $newPresence = Origin.views.$inlinePresenceMessage(presenceEvent);
                }
            }

            if (!joinable) {
                // Disable any join game links
                $history.find('button.join-game[data-user-id="' + userId + '"]').addClass('disabled');
            }

            if (!broadcasting) {
                // Disable any broadcast game links
                $history.find('button.broadcast-game[data-user-id="' + userId + '"]').addClass('disabled');
            }
        }

        if (!$newPresence) {
            return;
        }

        // Kill any previous presence events
        $takeLastEventIfMatches('[data-presence-user="' + userId + '"]');

        conditionalEventTimestamp(presenceEvent);

        // Add the new presence
        $newPresence.attr('data-presence-user', userId);
        updateHistoryWith($newPresence);
    };

    handleEventWith = function (evt, handler, handlerArg) {
        var $newEl = handler(handlerArg);
        conditionalEventTimestamp(evt);
        updateHistoryWith($newEl);

        return $newEl;
    };

    handleCompressingEventWith = function (evt, prevSelector, handler, user) {
        var $prevElement, $newElement, users = [], userIds;

        $prevElement = $takeLastEventIfMatches(prevSelector);

        if ($prevElement.length === 1) {
            $prevElement.attr('data-user-ids').split(',').forEach(function (userId) {
                users.push(originSocial.getUserById(userId));
            });
        }

        users.push(user);

        if (conversation.type === 'ONE_TO_ONE') {
            conditionalEventTimestamp(evt);
        }

        $newElement = handler(evt, users);

        userIds = users.map(function (user) {
            return user.id;
        });

        $newElement.attr('data-user-ids', userIds.join(','));
        updateHistoryWith($newElement);
    };

    handleSubscriptionStateEvent = function (evt) {
        if (evt.subscriptionState.direction === 'NONE') {
            if (!evt.previousSubscriptionState.pendingContactApproval &&
            evt.subscriptionState.pendingContactApproval) {
                // We sent them a subscription request
                handleEventWith(evt, Origin.views.$sentFriendRequestNotice, evt);
            }
            else if (!evt.previousSubscriptionState.pendingCurrentUserApproval &&
                 evt.subscriptionState.pendingCurrentUserApproval) {
                // They sent us a subscription request
                handleEventWith(evt, Origin.views.$nonFriendMessage, evt.user);
            }
        }
        else if (evt.previousSubscriptionState.pendingContactApproval &&
             (evt.subscriptionState.direction === 'BOTH')) {
            // There's a small window where the remote user can appear offline
            // while we wait for their initial presence to come in. Kill that
            // offline message once we figure out that they've approved our
            // subscription request
            $takeLastEventIfMatches('[data-presence-user="' + evt.user.id + '"]');

            // They accepted our subscription request
            handleEventWith(evt, Origin.views.$acceptedFriendRequestNotice, evt);
        }

        // Disable any stale actions
        if (evt.subscriptionState.pendingContactApproval ||
        evt.subscriptionState.pendingCurrentUserApproval ||
        (evt.subscriptionState.direction === 'BOTH')) {
            $history.find('span.send-request[data-user-id="' + evt.user.id + '"]').addClass('disabled');
        }

        if (!evt.subscriptionState.pendingCurrentUserApproval) {
            $history.find('span.accept-request[data-user-id="' + evt.user.id + '"]').addClass('disabled');
        }
    };

    handleMultiplayerInviteEvent = function (inviteEvent) {
        // Don't show the invites we send
        if (inviteEvent.from.id === originSocial.currentUser.id) {
            return;
        }

        $takeLastEventIfMatches('[data-presence-user="' + inviteEvent.from.id + '"],game');
        conditionalEventTimestamp(inviteEvent);

        // Remove any previous game-related notifications
        $history.find('li.game').remove();

        updateHistoryWith(Origin.views.$multiplayerInvite(inviteEvent));

        if (!inviteEvent.historic) {
            // Notify our parent window that something happened
            notifyParentWindow();
        }
    };

    handleRoomDestroyedEvent = function (roomDestroyedEvent) {
        if (conversation.finishReason === "USER_FINISHED_INVISIBLE") {
            conversation.leaveVoice();
            conversation.requestLeaveConversation();
        }
        else {
            Origin.views.$chatRoomDeletedNotice(roomDestroyedEvent).appendTo($history);
            updateInputField();
            emptyParticipantList();
            Origin.views.disableToolbarButtons();
            conversation.leaveVoice();
        }
    };

    // Voip call handlers
    handleCallIncoming = function (voipEvent) {
        if (!voipEvent.historic) {
            Origin.views.toolbarOnCallIncoming();
        }
    };

    handleVoipRemoteCalling = function (voipEvent) {
        if (conversation.type === 'ONE_TO_ONE') {
            if (!voipEvent.historic) {
                // Notify our parent window that something happened
                notifyParentWindow();
            }
        }
    };

    handleVoipLocalCalling = function (voipEvent) {
        if (conversation.type === 'ONE_TO_ONE') {
        }
    };

    handleVoipInputDeviceChange = function(inputDevice) {
        updateHistoryWith(Origin.views.$voipInputDeviceChangeNotice(inputDevice));
    }

    handleVoipCallStarted = function (voipEvent) {
        if (conversation.type === 'ONE_TO_ONE') {
            conditionalEventTimestamp(voipEvent);
            updateHistoryWith(Origin.views.$voipCallStartedNotice(voipEvent));
            if (!voipEvent.historic) {
                // Notify our parent window that something happened
                notifyParentWindow();
            }
        }
        else {
            updateHistoryWith(Origin.views.$voipCallStartedNotice(voipEvent));
        }
    };

    handleVoipCallEnded = function (voipEvent) {
        if (conversation.type === 'ONE_TO_ONE') {
            conditionalEventTimestamp(voipEvent);
            updateHistoryWith(Origin.views.$voipCallEndedNotice(voipEvent));
            if (!voipEvent.historic) {
                // Notify our parent window that something happened
                notifyParentWindow();
            }
        }
        else
        {
            updateHistoryWith(Origin.views.$voipCallEndedNotice(voipEvent));
        }
    };

    // PDH PDH
    handleVoipMissedCall = function (voipEvent) {
        if (conversation.type === 'ONE_TO_ONE') {
            // Remove the previous "Call Ended" message
            $history.children('li.system.voip-ended:last-child').remove();
            conditionalEventTimestamp(voipEvent);
            updateHistoryWith(Origin.views.$voipMissedCallNotice(voipEvent, voipEvent.from));
            if (!voipEvent.historic) {
                // Notify our parent window that something happened
                notifyParentWindow();
            }
        }
    };

    handleVoipNoAnswer = function (voipEvent) {
        if (conversation.type === 'ONE_TO_ONE') {
            // Remove the previous "Call Ended" message
            $history.children('li.system.voip-ended:last-child').remove();
            conditionalEventTimestamp(voipEvent);
            updateHistoryWith(Origin.views.$voipNoAnswerNotice(voipEvent, voipEvent.from));
            if (!voipEvent.historic) {
                // Notify our parent window that something happened
                notifyParentWindow();
            }
        }
    };

    handleVoipInactivity = function (voipEvent) {
        if (conversation.type === 'ONE_TO_ONE') {
            // Remove the previous "Call Ended" message
            $history.children('li.system.voip-ended:last-child').remove();
            conditionalEventTimestamp(voipEvent);
        }
        updateHistoryWith(Origin.views.$voipInactivityNotice(voipEvent, voipEvent.from));
        if (!voipEvent.historic) {
            // Notify our parent window that something happened
            notifyParentWindow();
        }
    };

    conditionalNonFriendMessage = function (convEvt) {
        var handler, user = convEvt.user;

        if (user.subscriptionState.pendingContactApproval) {
            handler = Origin.views.$pendingContactApprovalNotice;
        }
        else if (user.subscriptionState.direction !== 'BOTH') {
            handler = Origin.views.$nonFriendMessage;
        }

        if (handler) {
            handleEventWith(convEvt, handler, user);
        }
    };

    handleConversationEvent = function (convEvent) {
        convEvent = $.extend({ historic: false }, convEvent);

        switch (convEvent.type) {
            case 'MESSAGE':
                handleMessageEvent(convEvent);
                break;
            case 'CHAT_STATE_NOTIFICATION':
                handleChatStateNotificationEvent(convEvent);
                break;
            case 'PRESENCE_CHANGE':
                handlePresenceEvent(convEvent);
                break;
            case 'SUBSCRIPTION_STATE_CHANGE':
                handleSubscriptionStateEvent(convEvent);
                break;
            case 'MULTIPLAYER_INVITE':
                if (conversation.type === 'ONE_TO_ONE') {
                    handleMultiplayerInviteEvent(convEvent);
                }
                break;
            case 'PARTICIPANT_ENTER':
                if (conversation.type === 'ONE_TO_ONE') {
                    // Only show non-friend messages for 1:1 chats as they can be very
                    // noisy
                    conditionalNonFriendMessage(convEvent);
                }
                else if (conversation.type === "MULTI_USER") {
                    if (addParticipant(convEvent.user, false)) {
                        handleCompressingEventWith(convEvent, '.participant-enter', Origin.views.$participantsEnterNotice, convEvent.user);
                    }
                }
                break;
            case 'PARTICIPANT_EXIT':
                handleCompressingEventWith(convEvent, '.participant-exit', Origin.views.$participantsExitNotice, convEvent.user);
                if (conversation.type === "MULTI_USER") {
                    removeParticipant(convEvent.user);
                }
                break;
            case 'MUC_ROOM_DESTROYED':
                handleRoomDestroyedEvent(convEvent);
                break;

            // Chat voip messages   
            case 'VOICE_REMOTE_CALLING':
                handleCallIncoming(convEvent);
                if (conversation.type === 'ONE_TO_ONE') {
                    handleVoipRemoteCalling(convEvent);
                }
                break;
            case 'VOICE_LOCAL_CALLING':
                handleCallIncoming(convEvent);
                if (conversation.type === 'ONE_TO_ONE') {
                    handleVoipLocalCalling(convEvent);
                }
                break;
            case 'VOICE_STARTED':
                if (conversation.type === 'ONE_TO_ONE') {
                    handleVoipCallStarted(convEvent);
                    mostRecentChannelId = conversation.voiceChannel;
                }
                else {
                    handleVoipCallStarted(convEvent);
                    $participantItemForUserId(originSocial.currentUser.id).addClass("voip");
                    mostRecentChannelId = conversation.chatChannel.channelId;
                }
                break;
            case 'VOICE_ENDED':
                if (conversation.voiceCallState !== "VOICECALL_DISCONNECTED") // Hack to get around the multiple events we are getting
                {
                    handleVoipCallEnded(convEvent);
                }
                if (conversation.type !== 'ONE_TO_ONE') {
                    $participantItemForUserId(originSocial.currentUser.id).removeClass("voip local-mute remote-mute muted-by-admin admin-mute talking");
                }
                break;
            case 'VOICE_MISSED_CALL':
                handleVoipMissedCall(convEvent);
                break;
            case 'VOICE_NOANSWER':
                handleVoipNoAnswer(convEvent);
                break;
            case 'VOICE_INACTIVITY':
                handleVoipInactivity(convEvent);
                break;

            // Muting events   
            case 'VOICE_LOCAL_MUTED':
                if (conversation.type === 'ONE_TO_ONE') {
                    handleCompressingEventWith(convEvent, '.voip-local-muted', Origin.views.$voipCallMutedNotice, convEvent.from);
                }
                else {
                    $participantItemForUserId(convEvent.from.id).addClass("local-mute");
                    if (convEvent.from.id == originSocial.currentUser.id) {
                        handleCompressingEventWith(convEvent, '.voip-local-muted', Origin.views.$voipCallMutedNotice, convEvent.from);
                    }
                    else {
                        handleCompressingEventWith(convEvent, '.voip-local-muted-user', Origin.views.$voipCallMutedUserNotice, convEvent.from);
                    }
                    updateVoipToolTip(convEvent.from);
                }
                break;
            case 'VOICE_LOCAL_UNMUTED':
                if (conversation.type === 'ONE_TO_ONE') {
                    handleCompressingEventWith(convEvent, '.voip-local-unmuted', Origin.views.$voipCallUnmutedNotice, convEvent.from);
                }
                else {
                    $participantItemForUserId(convEvent.from.id).removeClass("local-mute muted-by-admin");
                    if (convEvent.from.id == originSocial.currentUser.id) {
                        handleCompressingEventWith(convEvent, '.voip-local-unmuted', Origin.views.$voipCallUnmutedNotice, convEvent.from);
                    }
                    else {
                        handleCompressingEventWith(convEvent, '.voip-local-unmuted-user', Origin.views.$voipCallUnmutedUserNotice, convEvent.from);
                    }
                    updateVoipToolTip(convEvent.from);
                }
                break;
            case 'VOICE_REMOTE_MUTED':
                if (conversation.type === 'MULTI_USER') {
                    $participantItemForUserId(convEvent.from.id).addClass("remote-mute");
                    updateVoipToolTip(convEvent.from);
                }
                break;
            case 'VOICE_REMOTE_UNMUTED':
                if (conversation.type === 'MULTI_USER') {
                    $participantItemForUserId(convEvent.from.id).removeClass("remote-mute admin-mute");
                    updateVoipToolTip(convEvent.from);
                }
                break;
            case 'VOICE_MUTED_BY_ADMIN': // For the user that got muted
                if (conversation.type === 'MULTI_USER') {
                    $participantItemForUserId(originSocial.currentUser.id).addClass("muted-by-admin");
                    handleCompressingEventWith(convEvent, '.voip-muted-by-admin', Origin.views.$voipCallMutedByAdminNotice, convEvent.from);
                    updateVoipToolTip(convEvent.from);
                }
                break;
            case 'VOICE_ADMIN_MUTED': // For the admin that muted
                if (conversation.type === 'MULTI_USER') {
                    $participantItemForUserId(convEvent.from.id).addClass("admin-mute");
                    handleCompressingEventWith(convEvent, '.voip-admin-muted', Origin.views.$voipCallAdminMutedNotice, convEvent.from);
                    updateVoipToolTip(convEvent.from);
                }
                break;

            // GROIP CHAT VOIP MESSAGES   
            case 'VOICE_JOIN':
                if (conversation.type === "MULTI_USER") {
                    $participantItemForUserId(convEvent.from.id).addClass("voip");
                }
                break;
            case 'VOICE_LEAVE':
                if (conversation.type === "MULTI_USER") {
                    $participantItemForUserId(convEvent.from.id).removeClass("voip local-mute remote-mute muted-by-admin admin-mute talking");
                    talking = false;
                    updateVoipToolTip(convEvent.from);
                }
                break;
            case 'VOICE_START_TALKING':
                if (conversation.type === "MULTI_USER") {
                    $participantItemForUserId(convEvent.from.id).addClass("voip talking");
                    talking = true;
                }
                break;
            case 'VOICE_STOP_TALKING':
                if (conversation.type === "MULTI_USER") {
                    $participantItemForUserId(convEvent.from.id).removeClass("talking");
                    talking = false;
                }
                break;
            case 'VOICE_START_FAIL':
                updateHistoryWith(Origin.views.$voipCallStartFailNotice(convEvent, convEvent.from));
                break;

            case 'VOICE_START_FAIL_UNDER_MAINTENANCE':
                updateHistoryWith(Origin.views.$voipCallStartFailUnderMaintenanceNotice(convEvent, convEvent.from));
                break;

            case 'VOICE_START_FAIL_VERSION_MISMATCH':
                updateHistoryWith(Origin.views.$voipCallStartFailVersionMismatch(convEvent, convEvent.from));
                if (conversation.type !== 'ONE_TO_ONE') {
                    $participantItemForUserId(originSocial.currentUser.id).removeClass("voip local-mute remote-mute muted-by-admin admin-mute talking");
                }
                break;

            case 'VOICE_GROUP_CALL_CONNECTING':
                break;

            default:
                console.warn('Unknown event type: ' + convEvent.type);
        }
    };

    setConversationTitle = function (title) {
        document.title = title;

        if (window.parent && window.parent.postMessage) {
            window.parent.postMessage({
                'type': 'setConversationTitle',
                'conversationId': conversationId,
                'title': title
            }, '*');
        }
    };

    setGroupTitle = function () {
        var title = tr('ebisu_client_chat_in').replace('%1', conversation.groupName).replace('%2', "");
        setConversationTitle(title);
    };

    updateInputField = function () {
        var isConversationActive = conversation.active, isConnectionEstablished = originSocial.connection.established;

        if (isConversationActive) {
            $textInput.toggleClass('disabled', !isConnectionEstablished);
            $textInput.parent().show();
            if ((isConnectionEstablished && !wasConnectionEstablished) || (isConversationActive && !wasConversationActive)) {
                $textInput.focus();
            }
        }
        else {
            if ((conversation.type === "DESTROYED") || (conversation.type === "KICKED") || (conversation.type === "LEFT_GROUP")) {
                // Prevent our height from changing when contenteditable is removed
                $textInput
                .css('height', window.getComputedStyle($textInput[0], null).getPropertyValue("height"))
                .removeAttr('contenteditable');

                $textInput.toggleClass('disabled', true);

                // Remove empty room notice if present
                $('li#empty-room-notice').remove();
            }
            else {
                $textInput.parent().hide();
            }
        }

        wasConversationActive = conversation.active;
        wasConnectionEstablished = originSocial.connection.established;
    };

    partnerNameChanged = function () {
        // Update our window title
        setConversationTitle(tr('ebisu_client_chat_with').replace('%1', oneToOnePartner.nickname));

        // Update any nickname links
        $history.find('span.nickname[data-user-id="' + oneToOnePartner.id + '"]')
        .text(oneToOnePartner.nickname);
    };

    // Update our title 
    switch (conversation.type) {
        case 'ONE_TO_ONE':
            oneToOnePartner.nameChanged.connect(partnerNameChanged);
            partnerNameChanged();
            break;

        case 'MULTI_USER':
            setGroupTitle();
            break;
    }

    Origin.views.$antiPhishingNotice().appendTo($history);

    if (conversation.type === 'MULTI_USER')
    {
        // Append the "invited by" message if appropriate
        if (conversation.chatChannel.invitedBy)
        {
            Origin.views.$invitedByNotice(conversation.chatChannel.invitedBy).appendTo($history);
        }

        // If nobody else is in the room prompt to invite others
        if (conversation.participants.length === 0)
        {
            Origin.views.$emptyRoomNotice().appendTo($history);
        }
    }

    updateInputField();

    $initialPresence = null;

    Origin.views.toolbarInit(conversation);

    // Set up a function to handle current user visibility changes
    onVisibilityChanged = function () {
        if (originSocial.currentUser.visibility === 'VISIBLE') {
            Origin.views.enableToolbarButtons();
        }
        else {
            Origin.views.disableToolbarButtons();
        }
    };

    // Connect local function to visibilityChanged SIGNAL
    originSocial.currentUser.visibilityChanged.connect(onVisibilityChanged);

    // Check user visibility upon initialization of conversation.
    onVisibilityChanged();

    // Constrain the size of our monologue elements
    setMonologueMaxWidth();

    // Set initial height values
    setParticipantListHeight();
    setHistoryListHeight();

    previousConversations = conversation.previousConversations;
    if (previousConversations.length) {
        // Get the last conversation
        lastConversation = previousConversations[previousConversations.length - 1];

        lastConversation.events.forEach(function (evt) {
            handleConversationEvent($.extend(evt, { historic: true }));
        });

        // Don't append new messages to the end of historic messages
        dialogueListItem = null;
    }

    if (oneToOnePartner) {
        // Show an initial message if the partner is offline or in-game
        if (oneToOnePartner.availability === 'UNAVAILABLE') {
            $initialPresence = Origin.views.$offlineRecipientNotice(oneToOnePartner);
        }
        else if (oneToOnePartner.playingGame) {
            // Remove any previous game-related notifications
            $history.find('li.game').remove();

            $initialPresence = Origin.views.$gamePresenceMessage(oneToOnePartner, oneToOnePartner.playingGame);
        }

        if ($initialPresence) {
            // Add the new presence
            $initialPresence
            .attr('data-presence-user', oneToOnePartner.id)
            .appendTo($history);
        }
    }

    // Handle all exisiting events
    conversation.events.forEach(handleConversationEvent);

    // Start listening for events
    conversation.eventAdded.connect(handleConversationEvent);

    // Listen for the conversation's request to be closed (this comes from the participant list context menu's "Leave Chat Room" option)
    conversation.leaveRequested.connect(function () {
        if (window.parent && window.parent.postMessage) {
            window.parent.postMessage({
                'type': 'closeConversation',
                'conversationId': conversationId
            }, '*');
        }
    });

    // Handle chat input
    /*jslint unparam: true*/
    $textInput.chatInput()
    .on('chatInput', function (evt, body) {
        clearTimeout(composingTimeout);
        composing = false;
        conversation.sendMessage(body, threadId);
        setTimeout(function () { handleMessageQueue(); }, 100);
        return false;
    })
    .on('keydown', function (evt) {
        window.setTimeout(function () {
            // Keep us scrolled to the bottom while typing
            scrollHistoryToBottom();
            return true;
        }, 0);

        if (evt.which === 9) {
            // Eat the tab event
            evt.stopImmediatePropagation();
            evt.preventDefault();
        }
    })
    .on('keyup', function (evt) {
        if ($textInput.text() === "") {
            clearTimeout(composingTimeout);
            composing = false;
            conversation.sendStopComposing();
        }
    })
    .on('composing', function () {
        if (composing) {
            clearTimeout(composingTimeout);
        }
        composingTimeout = window.setTimeout(function () {
            composing = false;
            conversation.sendPaused();
        }, 10000);

        if (!composing) {
            composing = true;
            conversation.sendComposing();
        }
    })
    .on('stopComposing', function () {
        if (composing) {
            clearTimeout(composingTimeout);
            composing = false;
            conversation.sendStopComposing();
        }
    });
    /*jslint unparam: false*/

    // Handle clicking the game name or box art
    $history.on('click', 'span.game-title.purchasable, img.box-art.purchasable', function () {
        var productId = $(this).attr('data-product-id');
        window.clientNavigation.showStoreProductPage(productId);
        return false;
    });

    // Open user-provided links in an external browser 
    $history.on('click', 'a.user-provided', function () {
        clientNavigation.launchExternalBrowser($(this).attr('href'));
        return false;
    });

    // Handle rejoin channel hotlink
    $history.on('click', '.rejoin-room-hotlink', function () {
        conversation.chatChannel.rejoinChannel();
    });

    // Handle various action links
    handleUserActionLink = function (actionName, handler) {
        $history.on('click', 'span.action.' + actionName + ':not(.disabled)', function () {
            var $action, user;

            $action = $(this);
            user = originSocial.getUserById($action.attr('data-user-id'));

            if (user) {
                handler.call(this, user);
                return false;
            }
        });
    };

    handleUserActionLink('block', function (user) {
        user.block();

        if (oneToOnePartner && (user.id === oneToOnePartner.id)) {
            // We blocked the only person we were talking to
            conversation.finish();
        }
        else {
            $(this).addClass('disabled');
        }
    });

    handleUserActionLink('accept-request', function (user) {
        user.acceptSubscriptionRequest();
    });

    handleUserActionLink('send-request', function (user) {
        user.requestSubscription();
    });

    handleChannelUserEvent = function (eventName, callback) {
        $('#participant-section').on(eventName, 'li.group-participant-list-item', function (evt) {
            var channelUser, chatChannel;

            channelUser = Origin.views.remoteUserForElement(this);

            if (channelUser) {
                // Grab the event
                evt.stopImmediatePropagation();
                evt.preventDefault();

                // Call our inner callback
                callback.call(this, channelUser, evt);
            }
        });
    };

    // Helper to attach an event handler that will look up a matching user
    // by the element's data-user-id attr
    handleHistoryUserEvent = function (eventName, selector, callback) {
        $history.on(eventName, selector, function (evt) {
            var userId, user;
            userId = $(this).attr('data-user-id');

            if (!userId) {
                return;
            }

            // Look up the user
            user = originSocial.getUserById(userId);

            if (user !== null) {
                callback(user);
            }

            evt.preventDefault();
            return false;
        });
    };

    // Invite members link
    $history.on('click', 'a.invite-members', function () {
        if (originSocial.roster.hasFriends) {            
            clientNavigation.showInviteFriendsToRoomDialog(conversation.groupGuid, conversation.chatChannel.channelId, conversation.id);
        } else {
            clientNavigation.showYouNeedFriendsDialog();
        }   
        return false;
    });

    // Update Origin link
    $history.on('click', 'a.update-link', function () {
        clientNavigation.launchExternalBrowser(tr('ebisu_client_help_article_on_updating_origin'));
        return false;
    });


    // VOIP survey link
    $history.on('click', 'a.take-voip-survey', function () {
        clientNavigation.showVoiceSurveyPrompt(mostRecentChannelId);
        return false;
    });

    // Handle menu
    handleChannelUserEvent('contextmenu', function (user) {
        showChannelUserMenu(user);

        return false;
    });

    $('#participant-section').on('click', 'li.group-participant-list-item button', function () {
        var user = Origin.views.remoteUserForElement(this.parentElement);
        showChannelUserMenu(user);

        return false;
    });

    showChannelUserMenu = function (user) {
        var $selected;
        if (!!conversation.chatChannel && !!conversation.chatGroup)
            Origin.ChatChannelMemberContextMenuManager.popup(user, conversation.chatChannel, conversation.chatGroup, conversation);
    };


    // Handle nickname and avatar clicks
    handleHistoryUserEvent('click', 'span.nickname, img.avatar', function (user) {
        user.showProfile("CHAT");
    });

    // Handle avatar context menus
    handleHistoryUserEvent('contextmenu', 'img.avatar', function (user) {
        if (user.id !== originSocial.currentUser.id) {
            Origin.RemoteUserContextMenuManager.popup([user], false);
        }
    });

    // Handle join game links
    handleHistoryUserEvent('click', 'button.join-game:not(.disabled)', function (user) {
        user.joinGame();
    });

    // Handle join broadcast links
    handleHistoryUserEvent('click', 'button.broadcast-game:not(.disabled)', function (user) {
        if (user.playingGame) {
            telemetryClient.sendBroadcastJoined('ConversationHistory');
            clientNavigation.launchExternalBrowser(user.playingGame.broadcastUrl);
        }
    });

    $history.on('contextmenu', function () {
        Origin.CopyContextMenuManager.popup();
        return false;
    });

    $textInput.on('contextmenu', function () {
        Origin.CutCopyPasteContextMenuManager.popup();
        return false;
    });

    $history.on('click', 'span.action.accept-voice-invite:not(.disabled)', function () {
        conversation.joinVoice();
        return false;
    });

    // Participant list click handlers

    // Handle clicking the game name
    $participantList.on('click', '.group-participant-list-item span.subtitle.otk-hyperlink', function () {
        var $listItem, user, userId;

        $listItem = $(this).closest('.group-participant-list-item');
        userId = $listItem.attr('data-user-id');
        user = originSocial.getUserById(userId);

        if (user.playingGame && user.playingGame.productId && user.playingGame.purchasable) {
            clientNavigation.showStoreProductPage(user.playingGame.productId);
        }

        return false;
    });

    // Handle clicking the voip icon
    $participantList.on('click', '.group-participant-list-item .icon.voip', function () {
        var $listItem, user, userId;

        $listItem = $(this).closest('.group-participant-list-item');

        if ($listItem.hasClass('remote-mute') || $listItem.hasClass('admin-mute') || pushToTalk) {
            // Not actionable in this state
            return;
        }

        userId = $listItem.attr('data-user-id');
        user = originSocial.getUserById(userId);

        // Mute/unmute
        if (userId == originSocial.currentUser.id) {
            if (conversation.voiceCallState.indexOf("MUTED") !== -1) {
                conversation.unmuteSelfInVoice();
            }
            else {
                conversation.muteSelfInVoice();
            }
        }
        else {
            if (conversation.remoteUserVoiceCallState(user).indexOf("REMOTE_VOICE_LOCAL_MUTE") !== -1) {
                conversation.unmuteUserInVoice(user);
            }
            else {
                conversation.muteUserInVoice(user);
            }
        }

        return false;
    });

    // Handle clicking the joinable icon
    $participantList.on('click', '.group-participant-list-item .icon.joinable', function () {
        var $listItem, user, userId;

        $listItem = $(this).closest('.group-participant-list-item');
        userId = $listItem.attr('data-user-id');
        user = originSocial.getUserById(userId);

        // Join the game
        user.joinGame();

        return false;
    });

    // Handle clicking the broadcasting icon
    $participantList.on('click', '.group-participant-list-item .icon.broadcasting', function () {
        var $listItem, user, userId;

        $listItem = $(this).closest('.group-participant-list-item');
        userId = $listItem.attr('data-user-id');
        user = originSocial.getUserById(userId);

        // Join the broadcast
        if (user.playingGame && user.playingGame.broadcastUrl) {
            clientNavigation.launchExternalBrowser(user.playingGame.broadcastUrl);
        }

        return false;
    });

    $participantList.on('dblclick', '.group-participant-list-item:not(.self)', function () {
        var $listItem, user, userId;

        $listItem = $(this).closest('.group-participant-list-item');
        userId = $listItem.attr('data-user-id');
        user = originSocial.getUserById(userId);

        if (user.subscriptionState.direction === "BOTH")
            user.startConversation();
        return false;
    });

    // Handle timestamps being switched on/off
    updateTimestampStyle = function () {
        if (originChat.showTimestamps) {
            $timestampStyle.text('');
        }
        else {
            // Nuke timestamps
            if (conversation.type === 'ONE_TO_ONE') {
                $timestampStyle.text('ol#history > li.time { display: none; }');
            }
            else // MULTI_USER
            {
                $timestampStyle.text('ol#history span.timestamp { display: none; }');
            }
        }
    };

    originChat.showTimestampsChanged.connect(updateTimestampStyle);
    updateTimestampStyle();

    onUpdateSettings = function (name, value) {
        if (name === "EnablePushToTalk") {
            if (value === true) {
                value = "true";
            }
            else if (value === false) {
                value = "false";
            }

            pushToTalk = (value === "true") ? true : false;
            $participantItemForUserId(originSocial.currentUser.id).toggleClass("ptt", pushToTalk);
            if (pushToTalk) {
                if (conversation.voiceCallState.indexOf("MUTED") !== -1) {
                    // If we enable push to talk while muted we should unmute ourselves
                    conversation.unmuteSelfInVoice();
                }
            }

            updateVoipToolTip(originSocial.currentUser);
        }
        else if (name === "VoiceInputDevice") {
            var inputDevice = clientSettings.readSetting("VoiceInputDevice");
            handleVoipInputDeviceChange(inputDevice);
        }
    };

    // We need to watch for when the PTT settings value changes
    clientSettings.updateSettings.connect(onUpdateSettings);
    onUpdateSettings("EnablePushToTalk", clientSettings.readBoolSetting("EnablePushToTalk"));

    // Handle disconnections from the XMPP server
    handleConnectionChange = function () {
    console.log("handleConnectionChange");
        if (originSocial.connection.established) {
            console.log("originSocial.connection.established");

            // Remove any messages about us being offline
            $('li#self-offline').remove();

            if (conversation.type === "ONE_TO_ONE") {
                $textInput
                .css('height', '')
                .attr('contenteditable', 'plaintext-only');

                Origin.views.enableToolbarButtons();
            } else {
                $('li#empty-room-notice a.invite-members').show();
                if (!conversation.chatChannel.inChannel) {
                    conversation.chatChannel.rejoinChannel();
                }
            }
        }
        else {
            // Prevent our height from changing when contenteditable is removed
            $textInput
            .css('height', window.getComputedStyle($textInput[0], null).getPropertyValue("height"))
            .removeAttr('contenteditable');

            if (conversation.type === "MULTI_USER") {
                // Kill the participant list
                emptyParticipantList();

                // Hide empty room invite hyperlink if present
                $('li#empty-room-notice a.invite-members').hide();
            }

            // Remove any previous messages about us being offline (edge case but still...)
            $('li#self-offline').remove();

            // Add a message about us being offline
            updateHistoryWith(Origin.views.$offlineNotice());

            conversation.leaveVoice();
        }

        updateInputField();
    };
    originSocial.connection.changed.connect(handleConnectionChange);
    if (!originSocial.connection.established) {
        handleConnectionChange();
    }

    // Listen for avatar changes
    conversation.participants.concat([originSocial.currentUser]).forEach(function (user) {
        var baseSelector = 'ol#history > li.dialogue > ol.monologue > img.avatar';

        if (conversation.type === "ONE_TO_ONE") {
            user.avatarChanged.connect(function () {
                // Update all avatars in the chat
                $(baseSelector + '[data-user-id="' + user.id + '"]').attr('src', user.avatarUrl);
            });
        }
    });

    if (conversation.type === "MULTI_USER") {
        conversation.chatGroup.enterMucRoomFailed.connect(function (status) {
            // Prevent our height from changing when contenteditable is removed
            $textInput
            .css('height', window.getComputedStyle($textInput[0], null).getPropertyValue("height"))
            .removeAttr('contenteditable')
            .addClass('disabled');

            // Add a message about the user being offline
            if (conversation.type === "MULTI_USER") {
                // Kill the participant list
                emptyParticipantList();
            }

            // Add a message about not entering room
            updateHistoryWith(Origin.views.$cannotEnterRoomNotice(status));
        });

        conversation.chatGroup.rejoinMucRoomSuccess.connect(function () {
            $textInput
            .css('height', '')
            .attr('contenteditable', 'plaintext-only')
            .removeClass('disabled');

            if (conversation.type === "MULTI_USER") {
                $history.find('.cannot-enter-room').remove();
                Origin.views.enableToolbarButtons();
                createParticipantList();
            }
        });
    }
    
    $textInput.on('input', Origin.utilities.throttle(
        function () {
            setHistoryListHeight();
        }
    , 100));

    $(window).resize(Origin.utilities.throttle(
        function () {
            setMonologueMaxWidth();
            setParticipantListHeight();
            setHistoryListHeight();
        }
    , 100));


    $(document).bind('click', function () {
        // If there is a text selection, don't do anything on click or it
        // will immediately remove the selection when we call $textInput.focus();
        // If there is not any text selected and the user clicks anywhere,
        // restore focus to the input field.
        if (window.getSelection().toString().length === 0) {
            $textInput.focus();
        }

        if (window.parent && window.parent.postMessage) {
            window.parent.postMessage({
                'type': 'conversationFocused',
                'conversationId': conversationId
            }, '*');
        }
    });

    $(window).on('message', function (ev) {
        ev = ev.originalEvent;
        switch (ev.data.type) {
            case 'conversationRaised':
                $textInput.focus();
                break;
            default:
                console.warn('Received unknown message of type ' + ev.data.type);
        }
    });

    // Keep us scrolled to the right place when resizing
    Origin.views.initConversationResizeScroll();

} (jQuery));
