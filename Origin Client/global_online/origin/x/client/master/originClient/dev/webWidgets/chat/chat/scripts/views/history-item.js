/*global console, dateFormat, jQuery, productQuery, tr, window */
(function ($) {
    "use strict";

    var $chatStateNotification, $inlinePresenceMessage,
        $offlineRecipientNotice, $gamePresenceMessage, $nicknameSpan, $gameTitleSpan,
        $multiplayerInvite, $gameRelatedItem, $pendingContactApprovalNotice,
        $nonFriendMessage, $sentFriendRequestNotice, $acceptedFriendRequestNotice,
        $participantsEnterNotice, $participantsExitNotice, $broadcastEndedMessage,
        $compressingUsersNotice, $antiPhishingNotice, $currentUserLeftNotice,
        $voipCallStartedNotice, $voipCallEndedNotice, $voipCallMutedNotice, $voipCallUnmutedNotice,
        $voipCallMutedUserNotice, $voipCallUnmutedUserNotice, $voipInputDeviceChangeNotice, monologueEntry,
        $voipCallMutedByAdminNotice, $voipCallAdminMutedNotice,
        $voipNoAnswerNotice, $voipMissedCallNotice, $voipInactivityNotice,
        $voipCallStartFailNotice, $voipCallStartFailUnderMaintenanceNotice, $voipCallStartFailVersionMismatchNotice,
        $offlineNotice, containsKorean, $cannotEnterRoomNotice, $invitedByNotice, $emptyRoomNotice;

    // Returns true if the string contains Korean characters. Otherwise returns false
    containsKorean = function (string) {
        var pattern = new RegExp("[\u3130-\u318F\uAC00-\uD7AF]");
        return pattern.test(string);
    };

    monologueEntry = function (messageEvent) {
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
        var $item;
        switch (chatStateEvent.state) {
            case 'COMPOSING':
                $item = $('<li class="composing">');
                break;
            case 'PAUSED':
                $item = $('<li class="idle">');
                break;
        }

        return $item;
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
        var $item, $gameTitle, $content, $actions, titleString, nameHtml,
        $boxArt, $boxArtBorder, query, broadcasting = null, $button, isEmbargoed;

        broadcasting = playingGame.broadcastUrl;

        //flag to determine if the game is an unreleased game and should not show title or product info
        isEmbargoed = playingGame.gameTitle === tr('ebisu_client_notranslate_embargomode_title');

        $item = $('<li class="game actionable">').addClass(type);
        $content = $('<section class="content">').appendTo($item);

        nameHtml = $nicknameSpan(user)[0].outerHTML;

        if (type === 'presence') {
            titleString = tr('ebisu_client_user_now_playing_game').replace('%1', nameHtml);
        }
        else if (type === 'invite') {
            titleString = tr('ebisu_invite_to_game_with_punctuation').replace('%1', nameHtml);
        }
        else if (type === 'broadcasting') {
            titleString = tr('ebisu_client_watch_broadcast_invite').replace('%1', nameHtml);
        }

        $('<h1>').html(titleString).appendTo($content);

        $gameTitle = $('<span class="game-title">');

        // Only make this clickable if we the game is purchasable and its not embargoed
        if (playingGame.purchasable && !isEmbargoed) {
            $gameTitle.addClass("purchasable");
            $gameTitle.attr('data-product-id', playingGame.productId);
        }

        // Game invites don't have titles - they're looked up in the closure below
        if (user.statusText) {
            $gameTitle.text(user.statusText);
        } else if (playingGame.gameTitle) {
            $gameTitle.text(playingGame.gameTitle);
        }

        $gameTitle.appendTo($content);

        $actions = $('<section class="actions">');

        if (broadcasting) {
            $button = $('<button type="button" class="button primary broadcast-game">')
            .attr('data-user-id', user.id)
            .appendTo($actions);

            $('<span class="broadcast-game action">')
            .text(tr('ebisu_client_watch'))
            .appendTo($button);
        }

        if (playingGame.joinable) {
            $button = $('<button type="button" class="button primary join-game">')
            .attr('data-user-id', user.id)
            .appendTo($actions);

            $('<span class="join-game action">')
            .text(tr('ebisu_client_accept_invite'))
            .appendTo($button);
        }

        $actions.appendTo($content);

        //only retrieve the product info if we have the product id and the game is a released game
        if (playingGame.purchasable && !isEmbargoed) {
            // Assume we have box art - nearly all products should
            // This prevents us from relaying out later
            $boxArt = $('<img class="box-art purchasable">');
            $boxArtBorder = $('<section class="primary-image">');

            $boxArt.attr('data-product-id', playingGame.productId);

            $boxArt.appendTo($boxArtBorder);
            $boxArtBorder.prependTo($item);

            // Request the actual box art
            query = productQuery.startQuery(playingGame.productId);
            query.succeeded.connect(function (info) {
                //If the playingGame object came from multiplayerInvite() it will be null. 
                if (user.statusText) {
                    $gameTitle.text(user.statusText);
                } else if (playingGame.gameTitle) {
                    $gameTitle.text(playingGame.gameTitle);
                } else {
                    $gameTitle.text(info.title);
                }

                if (info.boxartUrls.length > 0) {
                    $boxArt.attr('src', info.boxartUrls[0]);
                    $boxArt.on('error', function () {
                        // Broken box art
                        $boxArtBorder.remove();
                    });
                }
                else {
                    // We were wrong! Remove the box art.
                    $boxArtBorder.remove();
                }
            });

            query.failed.connect(function () {
                $boxArtBorder.remove();
            });
        }

        return $item;
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
        return $('<li class="system input-device-changed">').text(tr('ebisu_client_your_microphone_is_now').replace('%1', inputDevice));
    };

    $voipCallStartedNotice = function (voipEvent) {
        var pttEnabled = clientSettings.readBoolSetting("EnablePushToTalk"),
            pttString = clientSettings.readSetting("PushToTalkKeyString");
        if (pttEnabled) {
            if (pttString.indexOf("mouse") != -1) {
                pttButton = pttString.match(/\d+/);
                pttString = tr('ebisu_string_mouse_button_voip').replace("%1", pttButton);
            }
            return $('<li class="system voip-started">').text(tr('ebisu_client_joined_voice_chat_push_to_talk').replace('%1', pttString));
        }
        else {
            return $('<li class="system voip-started">').text(tr('ebisu_client_Call_Started'));
        }
    };

    $voipCallEndedNotice = function (voipEvent) {
        var $item, link;

        $item = $('<li class="system voip-ended">');

        link = $('<a class="take-voip-survey">').text(tr('ebisu_client_let_us_know'))[0].outerHTML;

        $item.html(tr('ebisu_client_Call_Ended') + " " + link);

        return $item;
    };

    $voipCallMutedNotice = function (event, users) {
        return $('<li class="system voip-muted">').text(tr('ebisu_client_you_are_now_muted')).addClass('voip-local-muted');
    };

    $voipCallUnmutedNotice = function (event, users) {
        return $('<li class="system voip-unmuted">').text(tr('ebisu_client_you_are_now_unmuted')).addClass('voip-local-muted');
    };

    $voipCallMutedUserNotice = function (event, users) {
        // Not used for 1:1 conversations
    };

    $voipCallUnmutedUserNotice = function (event, users) {
        // Not used for 1:1 conversations
    };

    $voipCallMutedByAdminNotice = function (event, users) {
        // Not used for 1:1 conversations
    };

    $voipCallAdminMutedNotice = function (event, users) {
        // Not used for 1:1 conversations
    };

    $voipCallStartFailNotice = function (event, users) {
        return $('<li class="system voip-call-start-fail">').text(tr('ebisu_client_usable_to_start_voice_chat'));
    };

    $voipCallStartFailUnderMaintenanceNotice = function (event, users) {
        return $('<li class="system voip-call-start-fail">').text(tr('ebisu_client_voice_server_shut_down'));
    };

    $voipCallStartFailVersionMismatchNotice = function (event, users) {
        var $item, link;

        $item = $('<li class="system voip-call-start-fail-version-mismatch">');
        link = $('<a class="update-link">').text(tr('ebisu_client_its_easy'))[0].outerHTML;
        $item.html(tr('ebisu_client_please_update_origin_to_use_this_feature') + " " + link);

        return $item;
    };

    $voipMissedCallNotice = function (event, users) {
        var nameHtml, titleHtml;

        nameHtml = $nicknameSpan(users)[0].outerHTML;

        titleHtml = tr('ebisu_client_you_missed_a_call_from').replace('%1', nameHtml);

        return $('<li class="system voip-missed-call">').html(titleHtml);
    };

    $voipNoAnswerNotice = function (event, users) {
        var nameHtml, titleHtml;

        nameHtml = $nicknameSpan(users)[0].outerHTML;

        titleHtml = tr('ebisu_client_did_not_answer').replace('%1', nameHtml);

        return $('<li class="system voip-no-answer">').html(titleHtml);
    };

    $voipInactivityNotice = function (event, users) {
        return $('<li class="system voip-inactivity">').text(tr('ebisu_client_voice_chat_ended_due_to_inactivity'));
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
        var $item, $content, $actions, nameHtml, titleHtml, avatarUrl, $avatar,
        $avatarBorder;

        nameHtml = $nicknameSpan(user)[0].outerHTML;

        titleHtml = tr('ebisu_client_soref_not_friends')
        .replace('%1', nameHtml);

        $item = $('<li class="non-friend actionable">');

        avatarUrl = user.avatarUrl;
        if (avatarUrl !== null) {
            $avatar = $('<img class="avatar">').attr('src', avatarUrl);
            $avatar.attr('data-user-id', user.id);
            $avatarBorder = $('<section class="primary-image">');

            $avatar.appendTo($avatarBorder);
            $avatarBorder.appendTo($item);
        }

        $content = $('<section class="content">').appendTo($item);

        $('<h1>').html(titleHtml).appendTo($content);

        $actions = $('<section class="actions">').appendTo($content);

        if (user.subscriptionState.pendingCurrentUserApproval) {
            $('<span class="accept-request action">')
            .attr('data-user-id', user.id)
            .text(tr('ebisu_client_soref_accept_request'))
            .appendTo($actions);

            $('<span class="block action">')
            .attr('data-user-id', user.id)
            .text(tr('ebisu_client_unfriend_and_block'))
            .appendTo($actions);
        }
        else {
            $('<span class="send-request action">')
            .attr('data-user-id', user.id)
            .text(tr('ebisu_client_soref_send_request'))
            .appendTo($actions);
        }

        return $item;
    };

    $sentFriendRequestNotice = function (event) {
        var nameHtml, message;

        nameHtml = $nicknameSpan(event.user)[0].outerHTML;
        message = tr("ebisu_client_soref_sent_request").replace('%1', nameHtml);

        return $('<li class="system sent-request">').html(message);
    };

    $acceptedFriendRequestNotice = function (event) {
        var nameHtml, message;

        nameHtml = $nicknameSpan(event.user)[0].outerHTML;
        message = tr("ebisu_client_friend_accepted_request").replace('%1', nameHtml);

        return $('<li class="system accepted-request">').html(message);
    };

    $nicknameSpan = function (user) {
        return $('<span class="nickname">').text(user.nickname);
    };

    $gameTitleSpan = function (user) {
        var $titleSpan;

        $titleSpan = $('<span class="game-title">');

        return $titleSpan.text(user.playingGame.gameTitle);
    };

    $compressingUsersNotice = function (users, singularString, pluralString) {
        var namesHtml, message;

        namesHtml = users.map(function (user) {
            return $nicknameSpan(user)[0].outerHTML;
        }).join(', ');

        if (users.length > 1) {
            message = pluralString.replace('%1', namesHtml);
        }
        else {
            message = singularString.replace('%1', namesHtml);
        }

        return $('<li class="system">').html(message);
    };

    $participantsEnterNotice = function (event, users) {
        return $compressingUsersNotice(
                users,
                tr('ebisu_client_group_chat_user_joined'),
                tr('ebisu_client_group_chat_users_joined')
           ).addClass('participant-enter');
    };

    $participantsExitNotice = function (event, users) {
        return $compressingUsersNotice(
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
        return $('<li class="system" id="phising-warning">').text(tr('ebisu_phishin_message'));
    };

    $invitedByNotice = function (username) {
        // Not used for 1:1
    };

    $emptyRoomNotice = function () {
        // Not used for 1:1
    };

    $currentUserLeftNotice = function () {
        return $('<li class="system current-user-left">').text(tr('ebisu_client_you_have_left_chat'));
    };

    $offlineNotice = function () {
        console.log("one to one offline");
        return $('<li class="system" id="self-offline">').text(tr('ebisu_client_you_are_offline'));
    };

    $cannotEnterRoomNotice = function (status) {
        console.log("cannot enter room");
        return $('<li class="system" id="cannot-enter-room">').text(tr('ebisu_chat_unable_to_rejoin_chat_room'));
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

} (jQuery));
