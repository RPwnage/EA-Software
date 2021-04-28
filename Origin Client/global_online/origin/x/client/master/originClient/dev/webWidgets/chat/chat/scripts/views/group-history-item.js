/*global console, dateFormat, jQuery, productQuery, tr, window */
(function ($) {
    "use strict";

    var $chatStateNotification, $inlinePresenceMessage, $offlineRecipientNotice,
    $gamePresenceMessage, $nicknameSpan, $gameTitleSpan, $multiplayerInvite, $gameRelatedItem,
    $pendingContactApprovalNotice, $nonFriendMessage, $sentFriendRequestNotice,
    $acceptedFriendRequestNotice, $participantsEnterNotice, $participantsExitNotice,
    $broadcastEndedMessage, monologueEntry,
    $voiceStartedNotice, $voiceJoinNotice, $voiceLeaveNotice,
    $compressingUsersNotice, $antiPhishingNotice, $chatRoomDeletedNotice,
    $currentUserLeftNotice, $voipInputDeviceChangeNotice,
    $voipCallStartedNotice, $voipCallEndedNotice, $voipCallMutedNotice, $voipCallUnmutedNotice,
    $voipCallMutedUserNotice, $voipCallUnmutedUserNotice,
    $voipCallMutedByAdminNotice, $voipCallAdminMutedNotice,
    $voipNoAnswerNotice, $voipMissedCallNotice, $voipInactivityNotice,
    $voipCallStartFailNotice, $voipCallStartFailUnderMaintenanceNotice, $voipCallStartFailVersionMismatchNotice,
    $offlineNotice, containsKorean, $cannotEnterRoomNotice, $invitedByNotice, $emptyRoomNotice;

    // Returns true if the string contains Korean characters. Otherwise returns false
    containsKorean = function (string) {
        var pattern = new RegExp("[\u3130-\u318F\uAC00-\uD7AF]");
        return pattern.test(string);
    };

    monologueEntry = function(messageEvent) {
        var messageListItem = document.createElement("li");
        messageListItem.className = "message";

        // Messages containing Korean characters require a particular font
        if (containsKorean(messageEvent.body)) {
            messageListItem.className += ' ko ';
        }
        
        messageListItem.innerHTML = Origin.utilities.escapeAndLinkifyHtml(messageEvent.body);
        return messageListItem;
    }
    
    $chatStateNotification = function (chatStateEvent) {
        // NO-OP
    };

    $inlinePresenceMessage = function (presenceEvent) {
        var presenceString, nameHtml;

        nameHtml = $nicknameSpan(presenceEvent.user)[0].outerHTML;

        switch (presenceEvent.user.availability) {
            case 'ONLINE':
            case 'CHAT':
                presenceString = tr('ebisu_client_friend_signing_in_message').replace('%1', nameHtml);
                break;
            case 'AWAY':
            case 'XA':
                presenceString = tr('ebisu_client_user_is_away').replace('%1', nameHtml);
                break;
            case 'UNAVAILABLE':
                presenceString = tr('ebisu_client_friend_signing_out_message').replace('%1', nameHtml);
                break;
        }

        return $('<li class="system presence">').html(presenceString);
    };

    $gameRelatedItem = function (user, type, playingGame) {
    };


    $gamePresenceMessage = function (user, playingGame) {
        var type = playingGame.broadcastUrl ? "broadcasting" : "presence";

        return $gameRelatedItem(user, type, playingGame);
    };

    $multiplayerInvite = function (inviteEvent) {
        // Make a fake playingGame object (Which is an OriginGameActivity Object in c++)
        var playingGame = {
            productId: inviteEvent.productId,
            joinable: true,
            purchasable: true //Can't invite to NOG games.
        };

        return $gameRelatedItem(inviteEvent.from, "invite", playingGame);
    };

    $voipInputDeviceChangeNotice = function (inputDevice) {

        var $item = $(
        '<li class="system input-device-changed">' +
            '<span class="nickname"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        $item.find('.message').text(tr('ebisu_client_your_microphone_is_now').replace('%1', inputDevice));
        $item.find('.timestamp').text(dateFormat.formatTime(new Date, false));

        return $item;
    };

    $voipCallStartedNotice = function (voipEvent) {
        var pttEnabled = clientSettings.readBoolSetting("EnablePushToTalk"),
            pttString = clientSettings.readSetting("PushToTalkKeyString");
        if (pttEnabled) {
            if (pttString.indexOf("mouse") != -1) {
                pttButton = pttString.match(/\d+/);
                pttString = tr('ebisu_string_mouse_button_voip').replace("%1", pttButton);
            }

            var $item = $(
            '<li class="system call-started">' +
                '<span class="nickname"></span>' +
                '<span class="message"></span>' +
                '<span class="timestamp"></span>' +
            '</li>'
            );

            $item.find('.message').text(tr('ebisu_client_joined_voice_chat_push_to_talk').replace('%1', pttString));
            $item.find('.timestamp').text(dateFormat.formatTime(voipEvent.time, false));

            return $item;
        }
    };

    $voipCallEndedNotice = function (voipEvent) {
        var link, $item = $(
        '<li class="system call-ended">' +
            '<span class="nickname"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        link = $('<a class="take-voip-survey">').text(tr('ebisu_client_let_us_know'))[0].outerHTML;

        $item.find('.message').html(tr('ebisu_client_Call_Ended') + " " + link);
        $item.find('.timestamp').text(dateFormat.formatTime(voipEvent.time, false));

        return $item;
    };

    $voipCallMutedNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_you_are_now_muted'),
                tr('ebisu_client_you_are_now_muted')
           ).addClass('voip-local-muted');
    };

    $voipCallUnmutedNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_you_are_now_unmuted'),
                tr('ebisu_client_you_are_now_unmuted')
           ).addClass('voip-local-unmuted');
    };

    $voipCallMutedUserNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_you_have_muted'),
                tr('ebisu_client_you_have_muted')
           ).addClass('voip-local-muted-user');
    };

    $voipCallUnmutedUserNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_you_have_unmuted'),
                tr('ebisu_client_you_have_unmuted')
           ).addClass('voip-local-unmuted-user');
    };

    $voipCallMutedByAdminNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_you_have_been_remotely_muted_by'),
                tr('ebisu_client_you_have_been_remotely_muted_by')
           ).addClass('voip-muted-by-admin');
    };

    $voipCallAdminMutedNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_you_have_remotely_muted'),
                tr('ebisu_client_you_have_remotely_muted')
           ).addClass('voip-admin-muted');
    };

    $voipCallStartFailNotice = function (event, users) {
        var $item = $(
        '<li class="system voip-call-start-fail">' +
            '<span class="nickname"></span>' +
            '<span class="icon"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        $item.find('.message').text(tr('ebisu_client_usable_to_start_voice_chat'));
        $item.find('.timestamp').text(dateFormat.formatTime(event.time, false));

        return $item;
    };

    $voipCallStartFailUnderMaintenanceNotice = function (event, users) {
        var $item = $(
        '<li class="system voip-call-start-fail">' +
            '<span class="nickname"></span>' +
            '<span class="icon"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        $item.find('.message').text(tr('ebisu_client_voice_server_shut_down'));
        $item.find('.timestamp').text(dateFormat.formatTime(event.time, false));

        return $item;
    };

    $voipCallStartFailVersionMismatchNotice = function (event, users) {
        var $item = $(
            '<li class="system voip-call-start-fail-version-mismatch">' +
                '<span class="nickname"></span>' +
                '<span class="icon"></span>' +
                '<span class="message"></span>' +
            '</li>'
            ),
            updateLinkHtml = $('<a class="update-link">').text(tr('ebisu_client_its_easy'))[0].outerHTML;
        $item.find('.message').html(tr('ebisu_client_please_update_origin_to_use_this_feature') + " " + updateLinkHtml);

        return $item;
    };

    $voipMissedCallNotice = function (event, users) {
        // Not used for group chats
    };

    $voipNoAnswerNotice = function (event, users) {
        // Not used for group chats
    };

    $voipInactivityNotice = function (event, users) {
        var $item = $(
        '<li class="system voip-inactivity">' +
            '<span class="nickname"></span>' +
            '<span class="icon"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        $item.find('.message').text(tr('ebisu_client_voice_chat_ended_due_to_inactivity'));
        $item.find('.timestamp').text(dateFormat.formatTime(event.time, false));

        return $item;
    };

    $offlineRecipientNotice = function (recipient) {
        var nameHtml, offlineMessage;

        nameHtml = $nicknameSpan(recipient)[0].outerHTML;

        offlineMessage = tr("ebisu_client_chat_offline_recipient")
        .replace('%1', nameHtml)
        .replace('%2', nameHtml);

        return $('<li class="system offline-recipient">').html(offlineMessage);
    };

    $pendingContactApprovalNotice = function (user) {
        var nameHtml, message;

        nameHtml = $nicknameSpan(user)[0].outerHTML;

        message = tr("ebisu_client_soref_not_friends").replace("%1", nameHtml) + " " +
              tr("ebisu_client_soref_request_pending_approval").replace("%1", nameHtml);

        return $('<li class="system pending-contact-approval">').html(message);
    };

    $nonFriendMessage = function (user) {
    };

    $sentFriendRequestNotice = function (event) {
        var $item, nameHtml, message;

        $item = $(
        '<li class="system sent-request">' +
            '<span class="nickname"></span>' +
            '<span class="icon"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        nameHtml = $nicknameSpan(event.user)[0].outerHTML;
        message = tr("ebisu_client_soref_sent_request").replace('%1', nameHtml);

        $item.find('.message').html(message);
        $item.find('.timestamp').text(dateFormat.formatTime(event.time, false));

        return $item;
    };

    $acceptedFriendRequestNotice = function (event) {
        var $item, nameHtml, message;

        $item = $(
        '<li class="system accepted-request">' +
            '<span class="nickname"></span>' +
            '<span class="icon"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        nameHtml = $nicknameSpan(event.user)[0].outerHTML;
        message = tr("ebisu_client_friend_accepted_request").replace('%1', nameHtml);

        $item.find('.message').html(message);
        $item.find('.timestamp').text(dateFormat.formatTime(event.time, false));

        return $item;
    };

    $nicknameSpan = function (user) {
        return $('<span class="nickname">').text(user.nickname);
    };

    $gameTitleSpan = function (user) {
        var $titleSpan;

        $titleSpan = $('<span class="game-title">');

        return $titleSpan.text(user.playingGame.gameTitle);
    };

    $compressingUsersNotice = function (event, users, singularString, pluralString) {
        var namesHtml, message, $item, uniqueUserMap = {}, uniqueUsers = [];

        // Ensure that each user is listed only once
        users.map(function (u) {
            if (uniqueUserMap[u.id] === undefined) {
                uniqueUserMap[u.id] = u;
            }
        });

        for (var key in uniqueUserMap) {
            uniqueUsers.push(uniqueUserMap[key]);
        };

        namesHtml = uniqueUsers.map(function (user) {
            return $nicknameSpan(user)[0].outerHTML;
        }).join(', ');

        if (uniqueUsers.length > 1) {
            message = pluralString.replace('%1', namesHtml);
        }
        else {
            message = singularString.replace('%1', namesHtml);
        }

        $item = $(
        '<li class="system">' +
            '<span class="nickname"></span>' +
            '<span class="icon"></span>' +
            '<span class="message"></span>' +
            '<span class="timestamp"></span>' +
        '</li>'
        );

        $item.find('.message').html(message);
        $item.find('.timestamp').text(dateFormat.formatTime(event.time, false));

        return $item;
    };

    $participantsEnterNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_group_chat_user_joined'),
                tr('ebisu_client_group_chat_users_joined')
           ).addClass('participant-enter');
    };

    $participantsExitNotice = function (event, users) {
        return $compressingUsersNotice(
                event,
                users,
                tr('ebisu_client_group_chat_user_left'),
                tr('ebisu_client_group_chat_users_left')
           ).addClass('participant-exit');
    };

    $broadcastEndedMessage = function (user) {
        var nameHtml, message, gameTitleHtml;

        nameHtml = $nicknameSpan(user)[0].outerHTML;
        gameTitleHtml = $gameTitleSpan(user)[0].outerHTML;
        message = tr('ebisu_client_user_stopped_broadcasting_inline').replace('%1', nameHtml).replace('%2', gameTitleHtml);

        return $('<li class="system broadcast-end">').html(message);
    };

    $antiPhishingNotice = function () {
        var $item = $(
        '<li class="system" id="phising-warning">' +
            '<span class="nickname"></span>' +
            '<span class="message"></span>' +
        // no timestamp for this
        '</li>'
        );

        $item.find('.message').html(tr('ebisu_phishin_message'));
        return $item;
    };

    $invitedByNotice = function (username) {
        var $item = $(
        '<li class="system" id="invited-by-notice">' +
            '<span class="nickname"></span>' +
            '<span class="message"></span>' +
        // no timestamp for this
        '</li>'
        );

        $item.find('.message').html(tr('ebisu_client_someone_invite_you_to_this_chat').replace('%1', username));
        return $item;
    };

    $emptyRoomNotice = function () {
        var inviteLink, $item = $(
        '<li class="system" id="empty-room-notice">' +
            '<span class="nickname"></span>' +
            '<span class="message"></span>' +
        // no timestamp for this
        '</li>'
        );

        inviteLink = $('<a class="invite-members">').text(tr('ebisu_client_invite_friends_into_this_chat_room'))[0].outerHTML;
        
        $item.find('.message').html(tr('ebisu_client_theres_no_one_in_your_chat_room_yet') + " " + inviteLink);
        return $item;
    };

    $chatRoomDeletedNotice = function (event) {
        var $item = $(
            '<li class="system" id="chat-room-deleted-notice">' +
                '<span class="nickname"></span>' +
                '<span class="message"></span>' +
                '<span class="timestamp"></span>' +
            '</li>'
            );

        $item.find('.message').html(event.message);
        $item.find('.timestamp').text(dateFormat.formatTime(event.time, false));
        return $item;
    };

    $currentUserLeftNotice = function () {
        return $('<li class="system current-user-left">').text(tr('ebisu_client_you_have_left_chat'));
    };

    $offlineNotice = function () {
        var $item = $(
            '<li class="system" id="self-offline">' +
                '<span class="nickname"></span>' +
                '<span class="message"></span>' +
                '<span class="timestamp"></span>' +
            '</li>'
        );

        $item.find('.message').html(tr('ebisu_client_you_are_offline'));

        // This doesn't come from a conversation event so we have to "create" a timestamp
        $item.find('.timestamp').text(dateFormat.formatTime(new Date, false));

        return $item;
    };

    $cannotEnterRoomNotice = function (status) {
        var $item = $(
            '<li class="system cannot-enter-room">' +
                '<span class="nickname"></span>' +
                '<span class="message"></span>' +
                '<span class="timestamp"></span>' +
            '</li>'
        );

        if (status === ChatroomEnteredStatus.CHATROOM_ENTERED_FAILURE_MAX_USERS) {
            $item.find('.message').text(tr('ebisu_client_this_room_is_full_please_try_joining_later'));
            $item.find('.message').append( $('<span class="rejoin-room-hotlink otk-hyperlink">' + tr('ebisu_client_join_the_chat_room') + '</span>') );
        } else {
            $item.find('.message').text(tr('ebisu_chat_unable_to_rejoin_chat_room'));
        }

        // This doesn't come from a conversation event so we have to "create" a timestamp
        $item.find('.timestamp').text(dateFormat.formatTime(new Date, false));

        return $item;
    };
    
    
    if (!window.Origin) { window.Origin = {}; }
    if (!window.Origin.views) { window.Origin.views = {}; }

    window.Origin.views.$chatStateNotification = $chatStateNotification;
    window.Origin.views.monologueEntry = monologueEntry;
    window.Origin.views.$multiplayerInvite = $multiplayerInvite;
    window.Origin.views.$inlinePresenceMessage = $inlinePresenceMessage;
    window.Origin.views.$gamePresenceMessage = $gamePresenceMessage;
    window.Origin.views.$broadcastEndedMessage = $broadcastEndedMessage;
    window.Origin.views.$offlineRecipientNotice = $offlineRecipientNotice;
    window.Origin.views.$pendingContactApprovalNotice = $pendingContactApprovalNotice;
    window.Origin.views.$nonFriendMessage = $nonFriendMessage;
    window.Origin.views.$sentFriendRequestNotice = $sentFriendRequestNotice;
    window.Origin.views.$acceptedFriendRequestNotice = $acceptedFriendRequestNotice;
    window.Origin.views.$participantsEnterNotice = $participantsEnterNotice;
    window.Origin.views.$participantsExitNotice = $participantsExitNotice;
    window.Origin.views.$antiPhishingNotice = $antiPhishingNotice;
    window.Origin.views.$invitedByNotice = $invitedByNotice;
    window.Origin.views.$emptyRoomNotice = $emptyRoomNotice;
    window.Origin.views.$chatRoomDeletedNotice = $chatRoomDeletedNotice;
    window.Origin.views.$currentUserLeftNotice = $currentUserLeftNotice;
    window.Origin.views.$voipCallStartedNotice = $voipCallStartedNotice;
    window.Origin.views.$voipCallEndedNotice = $voipCallEndedNotice;
    window.Origin.views.$voipCallMutedNotice = $voipCallMutedNotice;
    window.Origin.views.$voipCallUnmutedNotice = $voipCallUnmutedNotice;
    window.Origin.views.$voipCallMutedUserNotice = $voipCallMutedUserNotice;
    window.Origin.views.$voipCallUnmutedUserNotice = $voipCallUnmutedUserNotice;
    window.Origin.views.$voipCallMutedByAdminNotice = $voipCallMutedByAdminNotice;
    window.Origin.views.$voipCallAdminMutedNotice = $voipCallAdminMutedNotice;
    window.Origin.views.$voipCallStartFailNotice = $voipCallStartFailNotice;
    window.Origin.views.$voipCallStartFailUnderMaintenanceNotice = $voipCallStartFailUnderMaintenanceNotice;
    window.Origin.views.$voipCallStartFailVersionMismatch = $voipCallStartFailVersionMismatchNotice;
    window.Origin.views.$voipMissedCallNotice = $voipMissedCallNotice;
    window.Origin.views.$voipNoAnswerNotice = $voipNoAnswerNotice;
    window.Origin.views.$voipInactivityNotice = $voipInactivityNotice;
    window.Origin.views.$voipInputDeviceChangeNotice = $voipInputDeviceChangeNotice;
    window.Origin.views.$offlineNotice = $offlineNotice;
    window.Origin.views.$cannotEnterRoomNotice = $cannotEnterRoomNotice;

} (jQuery));
