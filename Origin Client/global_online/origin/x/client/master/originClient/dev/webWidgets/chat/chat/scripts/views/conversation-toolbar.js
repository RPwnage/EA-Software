/*global clientNavigation, clientSettings, console, document, jQuery, Origin, originConsole, originSocial, originVoice, parent, tr, window */
(function ($)
{
    'use strict';

    var conversation = null, $callout = null, $miniLevel, pushToTalk = false, talking = false, isVoiceLevelConnected = false,
        enableVoipButtonTimer = null, processVoipButtonClickEvent = true, enableVoipButtonTimeout = 500,
        calltimeSeconds, calltimeMinutes, calltimeHours, callQualityTimerId;

    function playIncomingRing() {
        if (parent.window.nativeChatWindow)
        {
            var windowScope = parent.window.nativeChatWindow.scope();
            if (windowScope === 0) {
                originVoice.playIncomingRing();
            }
        }
    }

    function playOutgoingRing() {
        if (parent.window.nativeChatWindow)
        {
            var windowScope = parent.window.nativeChatWindow.scope();
            if (windowScope === 0) {
                originVoice.playOutgoingRing();
            }
        }
    }

    function stopIncomingRing() {
        if (parent.window.nativeChatWindow)
        {
            var windowScope = parent.window.nativeChatWindow.scope();
            if (windowScope === 0) {
                originVoice.stopIncomingRing();
            }
        }
    }

    function stopOutgoingRing() {
        if (parent.window.nativeChatWindow)
        {
            var windowScope = parent.window.nativeChatWindow.scope();
            if (windowScope === 0) {
                originVoice.stopOutgoingRing();
            }
        }
    }

    function handleLocalAnswerVoiceCall()
    {
        stopIncomingRing();
        conversation.joinVoice();
        $('div#toolbar-message').empty();
    }

    function handleLocalHangUpIncomingVoiceCall()
    {
        conversation.leaveVoice();
    }

    function handleLocalHangUpOutgoingVoiceCall()
    {
        conversation.leaveVoice();
    }

    function handleVoiceCallStarted()
    {
        stopIncomingRing();
        stopOutgoingRing();
        $('div#toolbar-message').empty();
    }

    function handleVoiceCallEnded()
    {
        // Need to stop both ringers here
        stopIncomingRing();
        stopOutgoingRing();
        $('div#toolbar-message').empty();
    }

    function disableVoipButton()
    {
        $("#voip-button").parent(".otk-button").attr("disabled", "disabled");
        processVoipButtonClickEvent = false;
    }
    
    function enableVoipButton()
    {
        var $voipButton = $('#voip-button');
        var $voipButtonParent = $voipButton.parent(".otk-button");
        var isDisabled = ( typeof ($voipButtonParent.attr("disabled")) !== "undefined" );
        // canEnableVoipButton() - EBIBUGS-28148 & EBIBUGS-28163 We never want to enable the call button if the conversation isn't active.
        if (isDisabled && !enableVoipButtonTimer && canEnableVoipButton()) {
            enableVoipButtonTimer = setTimeout(function() { 
                $voipButtonParent.removeAttr("disabled"); 
                
                enableVoipButtonTimer = null;
                
                setTimeout(function() {
                    processVoipButtonClickEvent = true;
                }, enableVoipButtonTimeout); // extra timeout to allow clearing of QtWebkit cached click events
                
            }, enableVoipButtonTimeout);                
        }        
    }

    function canEnableVoipButton()
    {
        // EBIBUGS-28148 & EBIBUGS-28163 We never want to enable the call button if the conversation isn't active.
        var partnerHasVoip = false, partnerIsOnline = false, conversationActive = false;

        if(conversation)
        {
            conversationActive = conversation.active;
            if(conversation.type === 'ONE_TO_ONE')
            {
                partnerHasVoip = conversation.participants[0].capabilities.indexOf('VOIP') != -1;
                partnerIsOnline = conversation.participants[0].presence !== "OFFLINE";
            }
            else
            {
                partnerHasVoip = true;
                partnerIsOnline = true;
            }
        }

        return conversationActive
                && originVoice 
                && originVoice.supported 
                && partnerHasVoip 
                && partnerIsOnline 
                && originSocial.connection.established;
    }

    function setVoipButtonTitleAndIcon(title, icon)
    {
        var $voipButton = $('#voip-button');
        setTimeout(function() { 
            if (!!title && title.length > 0)
            {
                $voipButton.attr('title', title);
            }
            
            if (!!icon && icon.length > 0)
            {
                $('img#toolbar-voip-icon').attr('src', icon);
            }
            
        }, enableVoipButtonTimeout);                
    
    }    
    
    function handleIncomingVoiceCall(user)
    {
        var $answerHtml;
        
        playIncomingRing();

        $answerHtml = $('<span>').text(tr('ebisu_client_is_calling').replace('%1', user.nickname));
        $answerHtml.append($('<a class="otk-hyperlink" id="answerCall"></a>').text(tr('ebisu_client_answer')));
        $answerHtml.append($('<a class="otk-hyperlink" id="ignoreCall"></a>').text(tr('ebisu_client_ignore')));

        $('div#toolbar-message').empty().append($answerHtml);
        $('a#answerCall').click(handleLocalAnswerVoiceCall);
        $('a#ignoreCall').click(handleLocalHangUpIncomingVoiceCall);
    }

    function handleOutgoingVoiceCall(user)
    {
        var $callingHtml;

        playOutgoingRing();

        $callingHtml = $('<span>').text(tr('ebisu_client_calling').replace('%1', conversation.participants[0].nickname));
        $callingHtml.append($('<a class="otk-hyperlink" id="cancelCall"></a>').text(tr('ebisu_client_cancel')));

        $('div#toolbar-message').empty().append($callingHtml);
        $('a#cancelCall').click(handleLocalHangUpOutgoingVoiceCall);
    }

    function handleInviteFriendsButtonClick()
    {
        if (originSocial.roster.hasFriends) {            
            clientNavigation.showInviteFriendsToRoomDialog(conversation.groupGuid, conversation.chatChannel.channelId, conversation.id);
        } else {
            clientNavigation.showYouNeedFriendsDialog();
        }   
    }

    function handleMicSettingsButtonClick()
    {
        if ($('button#mic-settings').hasClass('no-button-treatment')) { return; } // treat as disabled

        // Toggle mute
        if (conversation.voiceCallState.indexOf('MUTED') !== -1) {
            conversation.unmuteSelfInVoice();
        }
        else {
            conversation.muteSelfInVoice();
        }
    }

    function handleVoipButtonClick()
    {
        if (processVoipButtonClickEvent) 
        {
            // Disable voip button when clicked to prevent spamming the button
            disableVoipButton();
            if ((conversation.voiceCallState === 'VOICECALL_INCOMING') ||
                (conversation.voiceCallState === 'VOICECALL_DISCONNECTED'))
            {
                // Transition to the hang-up icon immediately while it is ringing on the other side
                $('img#toolbar-voip-icon').attr('src', 'chat/images/toolbar/voip-end.png');
                conversation.joinVoice();

                if (conversation.type === 'MULTI_USER')
                {
                    // For group chats show "connecting" message
                    var $connectingHtml;
                    $connectingHtml = $('<span>').text(tr('ebisu_chat_joining_voice_chat'));
                    $('div#toolbar-message').empty().append($connectingHtml);
                }
            }
            else
            {
                setTimeout( function() { conversation.leaveVoice(); }, 250);
            }        
        }
    }

    function handleVoipDropdownClick()
    {
        // Prevent spamming the button
        $('#toolbar #voip-dropdown').attr('disabled', '');
        setTimeout(function () { $('#toolbar #voip-dropdown').removeAttr('disabled'); }, 500);

        Origin.ConversationToolbarVoiceMenuManager.popup(conversation);
    }

    function handleCreateGroupButtonClick()
    {
        clientNavigation.showCreateGroupDialog();
    }

    function handleConversationEvent(conversationEvent)
    {
        if (conversation.type === 'ONE_TO_ONE')
        {
            switch (conversationEvent.type) {
                case 'VOICE_REMOTE_CALLING':
                    handleIncomingVoiceCall(conversationEvent.from);

                    // Enable the voip button
                    enableVoipButton();
                    setVoipButtonTitleAndIcon(tr('ebisu_client_answer'));
                    break;

                case 'VOICE_LOCAL_CALLING':
                    handleOutgoingVoiceCall();
                    // Enable the voip button
                    enableVoipButton();
                    setVoipButtonTitleAndIcon(tr('ebisu_client_cancel'), 'chat/images/toolbar/voip-end.png');
                    break;

                case 'VOICE_ENDED':
                case 'VOICE_START_FAIL_VERSION_MISMATCH':
                    handleVoiceCallEnded();       
                
                    if (isVoiceLevelConnected)
                    {
                        originVoice.voiceLevel.disconnect(levelIndicatorUpdate);
                        isVoiceLevelConnected = false;
                    }

                    // Enable the voip button
                
                    // only set up tooltip if we are connected. On disconnect we clear out the tooltip
                    enableVoipButton();
                    if (originSocial.connection.established)
                    {
                        setVoipButtonTitleAndIcon(tr('ebisu_client_start_voice_chat'), 'chat/images/toolbar/voip-start.png');
                    } 
                    else
                    {
                        setVoipButtonTitleAndIcon(null, 'chat/images/toolbar/voip-start.png');                
                    }
                
                    $('img#toolbar-mic-settings-icon').attr('src', 'chat/images/toolbar/mic-settings.png');
                    $('button#mic-settings').hide();

                    callQualityAndTimerDestroy();
                    break;

                case 'VOICE_STARTED':
                    handleVoiceCallStarted();
                    originVoice.voiceLevel.connect(levelIndicatorUpdate);
                    isVoiceLevelConnected = true;
                
                    // Enable the voip button
                    enableVoipButton();
                    setVoipButtonTitleAndIcon(tr('ebisu_client_end_voice_chat'), 'chat/images/toolbar/voip-end.png');
                
                    $('img#toolbar-mic-settings-icon').attr('src', 'chat/images/toolbar/mic-settings.png');
                    updateMicSettingsButton();
                    $('button#mic-settings').show();

                    // Enable the call quality UI
                    callQualityAndTimerCreate();
                    break;

                case 'VOICE_MISSED_CALL':
                case 'VOICE_NOANSWER':
                case 'VOICE_INACTIVITY':
                    enableVoipButton();
                    if (originSocial.connection.established)
                    {
                        setVoipButtonTitleAndIcon(tr('ebisu_client_start_voice_chat'), 'chat/images/toolbar/voip-start.png');
                    }
                    else
                    {
                        setVoipButtonTitleAndIcon(null, 'chat/images/toolbar/voip-start.png');                
                    }
                    break;

                default: 
                    enableVoipButton();                
            }
        }
        else // MULTI_USER 
        {
            switch (conversationEvent.type) {
                case 'VOICE_ENDED':
                case 'VOICE_START_FAIL_VERSION_MISMATCH':
                    if (isVoiceLevelConnected) {
                        originVoice.voiceLevel.disconnect(levelIndicatorUpdate);
                        isVoiceLevelConnected = false;
                    }

                    // Enable the voip button
                    enableVoipButton();

                    // only set up tooltip if we are connected. On disconnect we clear out the tooltip
                    if (originSocial.connection.established) {
                        setVoipButtonTitleAndIcon(tr('ebisu_client_start_voice_chat'), 'chat/images/toolbar/voip-start.png');
                    }
                    else {
                        setVoipButtonTitleAndIcon(null, 'chat/images/toolbar/voip-start.png');
                    }

                    $('img#toolbar-mic-settings-icon').attr('src', 'chat/images/toolbar/mic-settings.png');
                    $('button#mic-settings').hide();

                    callQualityAndTimerDestroy();
                    break;

                case 'VOICE_STARTED':
                    originVoice.voiceLevel.connect(levelIndicatorUpdate);
                    isVoiceLevelConnected = true;

                    // Enable the voip button
                    enableVoipButton();
                    setVoipButtonTitleAndIcon(tr('ebisu_client_end_voice_chat'), 'chat/images/toolbar/voip-end.png');
                    updateMicSettingsButton();
                    $('img#toolbar-mic-settings-icon').attr('src', 'chat/images/toolbar/mic-settings.png');
                    $('button#mic-settings').show();

                    // Enable the call quality UI
                    callQualityAndTimerCreate();
                    break;

                default:
                    enableVoipButton();
                    break;
            }
        }

        // Update indicator on mic settings button icon
        if (conversationEvent.type === 'VOICE_LOCAL_MUTED')
        {
            if (conversationEvent.from.id == originSocial.currentUser.id)
            {
                $('img#toolbar-mic-settings-icon').attr('src', 'chat/images/toolbar/mic-settings-muted.png');
                updateMicSettingsButton();
            }
        }
        else if (conversationEvent.type === 'VOICE_MUTED_BY_ADMIN')
        {
            $('img#toolbar-mic-settings-icon').attr('src', 'chat/images/toolbar/mic-settings-muted.png');
            updateMicSettingsButton();
        }
        else if (conversationEvent.type === 'VOICE_LOCAL_UNMUTED')
        {
            if (conversationEvent.from.id == originSocial.currentUser.id)
            {
                $('img#toolbar-mic-settings-icon').attr('src', 'chat/images/toolbar/mic-settings.png');
                updateMicSettingsButton();
            }
        }
        else if (conversationEvent.type === 'VOICE_START_TALKING')
        {
            if (conversationEvent.from.id == originSocial.currentUser.id)
            {
                talking = true;
                $('button#mic-settings').addClass('talking');
            }
        }
        else if (conversationEvent.type === 'VOICE_STOP_TALKING')
        {
            if (conversationEvent.from.id == originSocial.currentUser.id)
            {
                talking = false;
                $('button#mic-settings').removeClass('talking');
            }
        }
        
    }

    function levelIndicatorUpdate(n)
    {
        if (originVoice && originVoice.supported) {
            if (conversation.voiceCallState === 'VOICECALL_CONNECTED') {
                $miniLevel.otkMiniLevelIndicator('setValue', n / 100.0);
            } else {
                $miniLevel.otkMiniLevelIndicator('setValue', 0);
            }
        }
    }

    function callQualityAndTimerCreate()
    {
        var $qualityMeter, $timer;

        $qualityMeter = $(
            '<div id="quality-meter">' +
                '<div id="bar1" class="quality-meter-bar"></div>' +
                '<div id="bar2" class="quality-meter-bar"></div>' +
                '<div id="bar3" class="quality-meter-bar"></div>' +
                '<div id="bar4" class="quality-meter-bar"></div>' +
            '</div>'
            );

        $timer = $('<span id="call-timer">00:00</span>');

        $('div#toolbar-message').empty();
        $('div#toolbar-message').append($qualityMeter);
        $('div#toolbar-message').append($timer);

        $qualityMeter.attr("quality", "4");

        calltimeSeconds = 0;
        calltimeMinutes = 0;
        calltimeHours = 0;
        callQualityTimerId = window.setInterval(updateCallTimerAndQualityMeter, 1000);
    }

    function callQualityAndTimerDestroy()
    {
        $('div#toolbar-message').empty();
        window.clearInterval(callQualityTimerId);
    }

    function updateCallTimerAndQualityMeter()
    {
        callTimerUpdate();
        callQualityUpdate();
    }

    function pad(n)
    {
        return (n < 10) ? ("0" + n) : n;
    }

    function callTimerUpdate()
    {
        var $timer = $('div#toolbar-message > #call-timer'), timestr;

        ++calltimeSeconds;
        if (calltimeSeconds >= 60)
        {
            calltimeSeconds = 0;
            ++calltimeMinutes;
            if (calltimeMinutes >= 60)
            {
                calltimeMinutes = 0;
                ++calltimeHours;
            }
        }

        if (calltimeHours > 0)
        {
            timestr = "" + calltimeHours + ":" + pad(calltimeMinutes) + ":" + pad(calltimeSeconds);
        }
        else
        {
            timestr = "" + pad(calltimeMinutes) + ":" + pad(calltimeSeconds);
        }

        $timer.text(timestr);
    }

    function callQualityUpdate()
    {
        var $qualityMeter = $('div#toolbar-message > #quality-meter'), quality;

        quality = 4;
        if (originVoice && originVoice.supported)
        {
            quality = originVoice.networkQuality + 1;
        }

        if ((quality === 1) || (quality === 2))
        {
            $qualityMeter.attr("title", tr("ebisu_client_network_connectivity_poor"));
        }
        else if (quality === 3)
        {
            $qualityMeter.attr("title", tr("ebisu_client_network_connectivity_ok"));
        }
        else if (quality === 4)
        {
            $qualityMeter.attr("title", tr("ebisu_client_network_connectivity_good"));
        }

        $qualityMeter.attr("quality", quality);
    }

    function handleVoipButtonChange() {
        if ( originVoice && originVoice.supported && // Show if voice supported, AND
             conversation.participants[0].capabilities.indexOf('VOIP') !== -1 && // if user has VOIP capabilities, AND
             conversation.participants[0].presence !== "UNKNOWN" && // if the user's presence is not 'UNKNOWN', AND
             conversation.participants[0].presence !== "OFFLINE" ) // if the user's presence is not 'OFFLINE'
        {
            $('span#voip-buttons').show();
        }
        else // otherwise hide the voip buttons
        {
            $('span#voip-buttons').hide();
        }
    }

    function toolbarOnCallIncoming()
    {
        clientSettings.writeSetting('Callout_VoipButtonOneToOneShown', true);
        clientSettings.writeSetting('Callout_VoipButtonGroupShown', true);
    }

    function updateMicSettingsButton()
    {
        var $micButton = $('button#mic-settings');
        $micButton.toggleClass('ptt', pushToTalk);
        if (pushToTalk)
        {
            // PTT enabled

            if (conversation.voiceCallState.indexOf('MUTED') !== -1)
            {
                // We are muted
                $micButton.removeClass('no-button-treatment');
                $micButton.attr('title', tr('Ebisu_client_unmute_your_microphone'));
            }
            else
            {
                $micButton.addClass('no-button-treatment');
                $micButton.removeAttr('title');
            }
        }
        else
        {
            // Voice activation

            $micButton.removeClass('no-button-treatment');

            if (conversation.voiceCallState.indexOf('MUTED') !== -1)
            {
                // We are muted
                $micButton.attr('title', tr('Ebisu_client_unmute_your_microphone'));
            }
            else
            {
                $micButton.attr('title', tr('ebisu_client_mute_your_microphone'));
            }
        }
    }

    function onUpdateSettings(name, value)
    {
        var title, content=[], line1={}, line2={}, $toolbarButtons, partnerHasVoip, partnerIsOnline, showVoipCallout;

        $toolbarButtons = $('#toolbar-buttons');

        if(conversation.type === 'ONE_TO_ONE')
        {
            partnerHasVoip = conversation.participants[0].capabilities.indexOf('VOIP') != -1;
            partnerIsOnline = conversation.participants[0].presence !== "OFFLINE";
        }
        else
        {
            partnerHasVoip = true;
            partnerIsOnline = true;
        }

        showVoipCallout = originVoice && originVoice.supported && partnerHasVoip && partnerIsOnline;

        if ($toolbarButtons && conversation && originSocial.connection.established
                && (((conversation.type === 'ONE_TO_ONE') && showVoipCallout) || (conversation.type === 'MULTI_USER'))
                )
        {
            // Some setting has changed - see if it's the callout settings
            if (((conversation.type === 'ONE_TO_ONE') && (name === 'Callout_VoipButtonOneToOneShown'))
                || ((conversation.type === 'MULTI_USER') && (name === 'Callout_VoipButtonGroupShown')))
            {
                if (value === true)
                {
                    value = 'true';
                }
                else if (value === false)
                {
                    value = 'false';
                }

                if (value === 'false')
                {
                    if ($callout)
                    {
                        // One already exists - kill it
                        $callout.remove();
                    }


                    if (conversation.type === 'ONE_TO_ONE')
                    {
                        title = tr('ebisu_client_voice_chat_title');
                        line1.text = tr('ebisu_client_reach_out_and_call_your_friend');
                        content.push(line1);
                    }
                    else
                    {
                        if (showVoipCallout)
                        {
                            // show both voip and invite features
                            title = tr('ebisu_client_voice_chat_and_invite_caps');

                            line1.text = tr('ebisu_client_join_voice_chat_to_hear_friends');
                            line1.imageUrl = "chat/images/callouts/voip.png";
                            content.push(line1);

                            line2.text = tr('ebisu_client_invite_additional_friends_to_chat');
                            line2.imageUrl = "chat/images/callouts/invites.png";
                            content.push(line2);
                        }
                        else
                        {
                            // only show invite feature
                            title = tr('invite_friends_uppercase');
                            line1.text = tr('ebisu_client_invite_additional_friends_to_chat');
                            content.push(line1);
                        }
                    }

                    $callout = $.otkCalloutCreate(
                    {
                        newTitle: tr('ebisu_client_new_title'),
                        title: title,
                        xOffset: 0,
                        yOffset: 22,
                        content: content,
                        bindedElement: $toolbarButtons
                    });

                    $callout.on('select', function (e)
                    {
                        // Mark it as shown
                        clientSettings.writeSetting(conversation.type === 'ONE_TO_ONE' ? 'Callout_VoipButtonOneToOneShown' : 'Callout_VoipButtonGroupShown', true);

                        // Nuke it from the DOM (the default is to just hide)
                        // Really we don't need this here anymore because it will be removed from the DOM below once
                        // the writeSetting(true) event hits, but I'm leaving it here anyway just to make sure. :-|
                        if ($callout)
                        {
                            $callout.remove();
                            $callout = null;
                        }
                    });
                }
                else
                {
                    // If we have a callout and this setting has changed to false, kill the callout.
                    // This covers the case where we have multiple tabs, each with a callout, and
                    // the user dismisses one of them. We want to dismiss ALL of them.
                    if ($callout)
                    {
                        $callout.remove();
                        $callout = null;
                    }
                }
            }
            else if (name === 'EnablePushToTalk')
            {
                if (value === true)
                {
                    value = 'true';
                }
                else if (value === false)
                {
                    value = 'false';
                }

                pushToTalk = (value === 'true') ? true : false;
                updateMicSettingsButton();
            }
        }
    }

    function toolbarInit(conv)
    {
        var voipButtonsShown = false;

        conversation = conv;

        //create the mini level.
        $miniLevel = $.otkMiniLevelIndicatorCreate('#toolbar-level-indicator',
        {
            numSegments: 5,
            orientation: 'horizontal'
        });

        $('button#mic-settings').hide();

        conversation.eventAdded.connect(handleConversationEvent);
        if (conversation.type === 'ONE_TO_ONE')
        {
            // Connect user's capability change event
            conversation.participants[0].capabilitiesChanged.connect(handleVoipButtonChange);
            // Connect user's presence change event
            conversation.participants[0].presenceChanged.connect(handleVoipButtonChange);
            // Set up initial voip buttons
            handleVoipButtonChange();

            if (conversation.voiceCallState === 'VOICECALL_INCOMING')
            {
                handleIncomingVoiceCall(conversation.participants[0]);
            }
        }
        else
        {   
            if (originVoice && originVoice.supported)
            {
                $('span#voip-buttons').show();
                voipButtonsShown = true;
            }
            else
            {
                $('span#voip-buttons').hide();
            }
        }

        $('#voip-button').attr('title', tr('ebisu_client_start_voice_chat'));
        $('#invite-friends').attr('title', tr('ebisu_client_invite_friends_into_this_chat_room'));
        $('#create-group-button').attr('title', tr('ebisu_client_create_chat_room'));

        $('#toolbar').on('click', '.otk-button:not([disabled]) > #voip-button', handleVoipButtonClick);

        $('#toolbar').on('click', '.otk-button:not([disabled]) > #voip-dropdown', handleVoipDropdownClick);

        // We need to watch for when the callout settings values change
        clientSettings.updateSettings.connect(onUpdateSettings);

        // Set our initial callout state
        // Don't show at first if it's an incoming message. If the user presses 'What's New'
        // in the client we would want to show it.
        if (conversation.creationReason !== 'INCOMING_MESSAGE')
        {
            if (conversation.type === 'ONE_TO_ONE')
            {
                onUpdateSettings('Callout_VoipButtonOneToOneShown', clientSettings.readBoolSetting('Callout_VoipButtonOneToOneShown'));
            }
            else
            {
                onUpdateSettings('Callout_VoipButtonGroupShown', clientSettings.readBoolSetting('Callout_VoipButtonGroupShown'));
            }
        }

        onUpdateSettings('EnablePushToTalk', clientSettings.readBoolSetting('EnablePushToTalk'));

    }

    function disableToolbarButtons()
    {
        // Add disabled attribute and clear tooltips
        $('.otk-button.toolbar').attr('disabled', '');
        if (originVoice && originVoice.supported) {
            $('#voip-button').removeAttr('title');
        }
        $('#invite-friends').removeAttr('title');
        $('#create-group-button').removeAttr('title');
    }

    function enableToolbarButtons()
    {
        // Remove disabled attribute and restore tooltips
        $('.otk-button.toolbar').removeAttr('disabled');
        if (originVoice && originVoice.supported) {
            $('#voip-button').attr('title', tr('ebisu_client_start_voice_chat'));
        }
        $('#invite-friends').attr('title', tr('ebisu_client_invite_friends_into_this_chat_room'));
        $('#create-group-button').attr('title', tr('ebisu_client_create_chat_room'));
    }

    
    // Handle disconnections from the XMPP server
    function handleConnectionChange()
    {
        if (originSocial.connection.established)
        {
            // we may need to show the callouts now that we're online
            if (conversation)
            {
                if (conversation.type === 'ONE_TO_ONE')
                {
                    onUpdateSettings('Callout_VoipButtonOneToOneShown', clientSettings.readBoolSetting('Callout_VoipButtonOneToOneShown'));
                }
                else
                {
                    onUpdateSettings('Callout_VoipButtonGroupShown', clientSettings.readBoolSetting('Callout_VoipButtonGroupShown'));
                }
            }
        }
        else
        {
            disableToolbarButtons();

            // Kill any calling messages, just in case
            $('div#toolbar-message').empty();
        }
    }

    originSocial.connection.changed.connect(handleConnectionChange);
    handleConnectionChange();


    if (!window.Origin) { window.Origin = {}; }
    if (!window.Origin.views) { window.Origin.views = {}; }

    //toolbar init
    window.Origin.views.toolbarInit = toolbarInit;
    window.Origin.views.disableToolbarButtons = disableToolbarButtons;
    window.Origin.views.enableToolbarButtons = enableToolbarButtons;
    window.Origin.views.toolbarOnCallIncoming = toolbarOnCallIncoming;

    $(document).ready(function ()
    {
        if (originVoice && originVoice.supported === false) {
            $('#voip-button').addClass('disabled').hide();
            $('#voip-dropdown').addClass('disabled').hide();
        }

        $('button#invite-friends').on('click', handleInviteFriendsButtonClick);
        $('button#mic-settings').on('click', handleMicSettingsButtonClick);
        $('button#create-group-button').on('click', handleCreateGroupButtonClick);
    });

} (jQuery));
