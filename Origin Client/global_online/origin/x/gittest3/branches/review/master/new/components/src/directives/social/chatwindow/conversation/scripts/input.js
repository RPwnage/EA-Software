(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationInputCtrl($scope, ConversationFactory) {
        var $element;

        this.setElement = function($el) {
            $element = $el;
            $element.attr('disabled', !Origin.auth.isOnline());
        };

        // Send the message
        this.sendMessage = function(message) {
            if ($scope.conversation.state === 'ONE_ON_ONE') {
                ConversationFactory.sendXmppMessage($scope.conversation.participants[0], message);
            }
            else {
                ConversationFactory.sendMultiXmppMessage($scope.conversation.id, message);
            }
        };

        function onClientOnlineStateChanged(online) {
            $element.attr('disabled', !online);
        }

        function onDestroy() {
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
        }

        //listen for connection change (for embedded)
        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
        $scope.$on('$destroy', onDestroy);
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
        .directive('originSocialChatwindowConversationInput', function(ComponentsConfigFactory, ConversationFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationInputCtrl',
                scope: {
                    conversation: '=',
                    placeholder: '@placeholder'
                },
                link: function(scope, element, attrs, ctrl) {
                    var $textInput = $(element).find('.origin-social-chatwindow-conversation-input'),
                        $sizingDiv = $(element).find('.origin-social-chatwindow-conversation-input-hidden-size-div'),
                        typing = false,
                        typingTimerId = 0;

                    ctrl.setElement($textInput);

                    $textInput.keydown(function(event){

                        var text = $textInput.val();

                        if((event.keyCode || event.which) === 13) {
                            event.stopImmediatePropagation();
                            event.preventDefault();
                            if (text.length) {
                                ctrl.sendMessage(text);
                                $textInput.val('');
                                $(element).trigger('changed');
                                typing = false;
                                if (typingTimerId !== 0) {
                                    clearTimeout(typingTimerId);
                                    typingTimerId = 0;
                                }
                            }
                        }
                    });


                    $textInput.keyup(function(event){

                        event = event;

                        var text = $textInput.val();

                        // Resize to fit the text
                        $sizingDiv.text(text);
                        var height = $sizingDiv.outerHeight();
                        $textInput.css({ height: height + 'px' });
                        $(element).trigger('changed');

                        //
                        // Handle the typing indicator

                        if (!typing)
                        {
                            typing = true;
                            ConversationFactory.sendXmppTypingState('composing', scope.conversation.participants[0]);
                        }

                        if (typingTimerId !== 0) {
                            clearTimeout(typingTimerId);
                            typingTimerId = 0;
                        }
                        typingTimerId = setTimeout(function() {
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

