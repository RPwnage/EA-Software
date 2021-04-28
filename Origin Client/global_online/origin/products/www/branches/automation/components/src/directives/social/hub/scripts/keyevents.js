/**
 * For all those who love to press buttons... I present key controls.
 * This is the top-level directive that provides defaults for keyboard events
 * If the [keytarget] attribute is present, the following defaults will be
 * applied to the element unless overridden.
 *
 * If an element needs to expand on this functionality, it can catch
 * the keyboard event locally and override it with a custom event handler
 * (see the friend.js directive for an example)
 *
 * @file keyboardevents.js
 */

(function() {
    'use strict';

    /**
     * The directive
     */
    function OriginSocialHubKeyevents(KeyEventsHelper, ActiveElementHelper, VoiceFactory, ConversationFactory, RosterDataFactory) {

        /**
         * The link
         */
        function OriginSocialHubKeyeventsLink(scope) {

            /*
             * Default callback handlers
             */

            // Rejects the incoming call with keyboard
            function handleVoiceEndRejectShortcut() {
                var voiceCallData = VoiceFactory.voiceCallObservable().data;
                var activeConverstation = Number(ConversationFactory.getActiveConversationId());
                var activeCallState = VoiceFactory.voiceCallInfoForConversation(activeConverstation).state;
                if (voiceCallData.hasIncomingCall && activeCallState === 'INCOMING') {
                    Origin.voice.ignoreCall(activeConverstation);
                    return false;
                } else if (voiceCallData.isCalling && activeCallState === 'OUTGOING') {
                    Origin.voice.leaveVoice(activeConverstation);
                    return false;
                } else if (voiceCallData.isInVoice && activeCallState === 'STARTED') {
                    Origin.voice.leaveVoice(activeConverstation);
                    return false;
                }
                
            }
            // Accepts the incoming call with keyboard
            function handleJoinAnswerCall() {
                var activeConverstation = Number(ConversationFactory.getActiveConversationId());
                var activeCallState = VoiceFactory.voiceCallInfoForConversation(activeConverstation).state;
                if (VoiceFactory.voiceCallObservable().data.hasIncomingCall && activeCallState === 'INCOMING') { //Answer Call
                    Origin.voice.answerCall(activeConverstation);
                    return false;
                } else if (!VoiceFactory.voiceCallObservable().data.hasIncomingCall) { // Join Call
                    var friend = RosterDataFactory.getRosterFriend(activeConverstation);
                    var conversations = ConversationFactory.getConversations();
                    if(friend.presence.show !== 'UNAVAILABLE') {
                        Origin.voice.joinVoice(activeConverstation, conversations[activeConverstation].participants);
                        return false;
                    }
                }
            }
            // Default for down keypress
            function handleKeypressDown() {
                var next = ActiveElementHelper.getNext(document.activeElement, document.activeElement.attributes.keytarget ? document.activeElement.attributes.keytarget.value === 'contextmenuitem' : false);
                angular.element(next).focus();
                return false;
            }
            // Default for up keypress
            function handleKeypressUp() {
                var prev = ActiveElementHelper.getPrev(document.activeElement, document.activeElement.attributes.keytarget ? document.activeElement.attributes.keytarget.value === 'contextmenuitem' : false);
                angular.element(prev).focus();
                return false;
            }
            // Default for right keypress
            function handleKeypressRight() {
                if (document.activeElement.attributes.keytarget.value === 'contextmenu') {
                    handleKeypressEnter();
                    return false;
                }
            }
            // Default for left keypress
            function handleKeypressLeft() {
                if (ActiveElementHelper.isActiveElementMenuOrMenuItem()) {
                    var menuRoot = angular.element(document.activeElement).closest('[keytarget=contextmenu]');
                    menuRoot.find('.otkdropdown-wrap').removeClass('otkdropdown-isvisible');
                    menuRoot.find('.otkcontextmenu-wrap').removeClass('otkcontextmenu-isvisible');
                    menuRoot.focus();
                    return false;
                }
            }
            // Default for enter keypress
            function handleKeypressEnter() {
                var clickElement = angular.element(document.activeElement).find('[keytargetenter],[keytarget]');

                if (clickElement.length && (clickElement[0].getAttribute('keytargetenter') !== null)) {
                    clickElement[0].click();
                } else {
                    document.activeElement.click();
                }
                return false;
            }
            // Stops the event if keydown event is not part
            // of keyboard accessible characters
            function onKeyDown(event) {
                if (KeyEventsHelper.isNotCharacterCode(event)) {
                    return false;
                }
            }
            // Maps events on keyup to default functions
            function onKeyUp(event) {
                var shiftActive = event.shiftKey,
                    ctrlActive = event.ctrlKey;

                if (shiftActive && ctrlActive) {
                    return KeyEventsHelper.mapEvents(event, {
                        'enter': handleJoinAnswerCall,
                        'backspace': handleVoiceEndRejectShortcut
                    });
                } else {
                    return KeyEventsHelper.mapEvents(event, {
                        'down': handleKeypressDown,
                        'up': handleKeypressUp,
                        'numpadPlus': handleKeypressRight,
                        'right': handleKeypressRight,
                        'left': handleKeypressLeft,
                        'numpadSubtract': handleKeypressLeft,
                        'tab': function() {
                            if (!shiftActive) {
                                handleKeypressDown();
                            } else {
                                handleKeypressUp();
                            }
                        },
                        'enter': handleKeypressEnter,
                        'escape': handleKeypressLeft,
                        'backspace': handleKeypressLeft
                    });

                }
            }

            function onDestroy() {
                /* Only unsubscribe to events if its client embedded browser. See Below subscribe event*/
                if(Origin.client.isEmbeddedBrowser()){
                    angular.element(document).off('keydown', onKeyDown);
                    angular.element(document).off('keyup', onKeyUp);
                }
            }

            /* Only subscribe to events if its client embedded browser.
               ORPS-4709 For now we are fixing it only for WEB, there is another ticket filed with design
               to reevaluate keyup and keydown events for embedded browser
            */
            if(Origin.client.isEmbeddedBrowser()){
                angular.element(document).on('keydown', onKeyDown);
                angular.element(document).on('keyup', onKeyUp);
            }

            window.addEventListener('unload', onDestroy);
            scope.$on('$destroy', onDestroy);

        }

        return {
            restrict: 'A',
            link: OriginSocialHubKeyeventsLink
        };
    }

/**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubKeyevents
     * @restrict A
     * @element ANY
     */
    angular.module('origin-components')
        .directive('originSocialHubKeyevents', OriginSocialHubKeyevents);
}());
