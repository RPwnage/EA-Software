(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationInputCtrl($scope, ConversationFactory, RosterDataFactory, AuthFactory) {
        var $element;

        this.setElement = function ($el) {
            $element = $el;
            $element.attr('disabled', !AuthFactory.isClientOnline());
        };

        // Send the message
        this.sendMessage = function (message) {
            if ($scope.conversation.state === 'ONE_ON_ONE') {
                ConversationFactory.sendXmppMessage($scope.conversation.participants[0], message);
            }
            else {
                ConversationFactory.sendMultiXmppMessage($scope.conversation.id, message);
            }
        };

        function onClientOnlineStateChanged(onlineObj) {
            $element.attr('disabled', (onlineObj && !onlineObj.isOnline));
            $scope.$digest();
        }

        function onXmppConnect() {
            onClientOnlineStateChanged({isLoggedIn: true,
                                        isOnline:true});
        }

        function onXmppDisconnect() {
            onClientOnlineStateChanged({isLoggedIn: true,
                                        isOnline:false});
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            RosterDataFactory.events.off('xmppConnected', onXmppConnect);
            RosterDataFactory.events.off('xmppDisconnected', onXmppDisconnect);
        }
        $scope.$on('$destroy', onDestroy);

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        // listen for xmpp connect/disconnect
        RosterDataFactory.events.on('xmppConnected', onXmppConnect);
        RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationInput
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @param {string} placeholder "Send a message" placeholder
     * @description
     *
     * origin chatwindow -> conversation -> input field
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-input
     *            conversation="conversation"
     *            placeholder="Send a message"
     *         ></origin-social-chatwindow-conversation-input>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationInputCtrl', OriginSocialChatwindowConversationInputCtrl)
        .directive('originSocialChatwindowConversationInput', function (ComponentsConfigFactory, ConversationFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationInputCtrl',
                scope: {
                    conversation: '=',
                    placeholder: '@placeholder'
                },
                link: function (scope, element, attrs, ctrl) {
                    var $textInput = $(element).find('.origin-social-chatwindow-conversation-input'),
                        $sizingDiv = $(element).find('.origin-social-chatwindow-conversation-input-hidden-size-div'),
                        typing = false,
                        typingTimerId = 0,
                        lastHeight = 0,
                        maxHeight = parseInt($textInput.css("max-height"), 10);

                    ctrl.setElement($textInput);

                    $textInput.keydown(function (event) {

                        var text = $textInput.val();

                        // Eat the ESCAPE key
                        if ((event.keyCode || event.which) === 27) {
                            event.stopImmediatePropagation();
                            event.preventDefault();
                        }

                        // ENTER key
                        if ((event.keyCode || event.which) === 13) {
                            event.stopImmediatePropagation();
                            event.preventDefault();
                            if (text.length) {
                                ctrl.sendMessage(text);
                                $textInput.val('');
                                $(element).trigger('resized');
                                typing = false;
                                if (typingTimerId !== 0) {
                                    clearTimeout(typingTimerId);
                                    typingTimerId = 0;
                                }
                            }
                        }
                    });


                    $textInput.keyup(function (event) {

                        event = event;

                        var text = $textInput.val();

                        // Resize to fit the text
                        $sizingDiv.text(text);
                        var height = $sizingDiv.outerHeight();
                        if (height !== lastHeight) {
                            lastHeight = height;
                            $textInput.css({ height: height + 'px' });
                            $(element).trigger('resized');

                            // Turn scrollbars on and off as needed
                            if (height >= maxHeight) {
                                $textInput.css({ 'overflow-y': 'scroll', 'padding': '10px 10px 0 10px' });
                            }
                            else {
                                $textInput.css({ 'overflow-y': 'hidden', 'padding': '10px' });
                            }
                        }

                        // Save the text inputed by the user
                        scope.conversation.pendingText = text;

                        //
                        // Handle the typing indicator

                        // check to see if we have text before sending the composing message
                        if (!typing && text.length !== 0)
                        {
                            typing = true;
                            ConversationFactory.sendXmppTypingState('composing', scope.conversation.participants[0]);
                        }

                        if (typingTimerId !== 0) {
                            clearTimeout(typingTimerId);
                            typingTimerId = 0;
                        }
                        typingTimerId = setTimeout(function () {
                            if (text.length) {
                                ConversationFactory.sendXmppTypingState('paused', scope.conversation.participants[0]);
                            }
                            else {
                                ConversationFactory.sendXmppTypingState('active', scope.conversation.participants[0]);
                            }
                            typing = false;
                        }, 10000);

                        if (text.length === 0) {
                            if (typing) {
                                typing = false;
                                ConversationFactory.sendXmppTypingState('active', scope.conversation.participants[0]);
                            }
                            if (typingTimerId !== 0) {
                                clearTimeout(typingTimerId);
                                typingTimerId = 0;
                            }
                        }
                    });


                    $textInput.focus();

                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/input.html'),
            };

        });
}());
