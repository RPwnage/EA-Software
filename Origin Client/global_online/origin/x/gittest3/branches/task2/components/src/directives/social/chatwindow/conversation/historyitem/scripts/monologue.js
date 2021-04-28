(function () {

    'use strict';
    
    var MONOLOGUE_BUBBLE_MAXWIDTH_SUBTRACT = 150;

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationHistoryitemMonologueCtrl($scope, $window, RosterDataFactory, UserDataFactory, ConversationFactory, NavigationFactory) {
        var $element, $parentHistory, $bubbleWidthStyles;

        if (typeof $scope.monologue === 'undefined') {
            // used for typing indicator
            $scope.monologue = {
                from: $scope.conversation.participants[0],
                messages: []
            };
        }

        $bubbleWidthStyles = $('<style type="text/css">').appendTo($('head'));

        this.setElement = function(el) {            
            $element = el;
            $parentHistory = $element.closest('.origin-social-chatwindow-conversation-history');
        };

        this.clickUser = function() {
            //NavigationFactory.goUserProfile($scope.monologue.from);
            // For now open a Dialog until the Proile page is back up and working
            $window.alert('Profile is not yet implemented');
        };

        this.clickURL = function(url) {
            NavigationFactory.asyncOpenUrl(url);
        };

        $scope.user = {};
            
        RosterDataFactory.getFriendInfo($scope.monologue.from).then(function(user) {
            if (typeof user !== 'undefined') {
                $scope.user = user;
                $scope.$digest();
            }
        });        

        function requestAvatar(nucleusId) {
            UserDataFactory.getAvatar(nucleusId, 'AVATAR_SIZE_MEDIUM')
                .then(function(response) {                
                    $scope.user.avatarImgSrc = response;
                    $scope.$digest();
                }, function() {
                    
                }).catch(function(error) {
                    Origin.log.exception(error, 'OriginSocialChatwindowConversationHistoryitemMonologueCtrl: UserDataFactory.getAvatar failed');
                });
        }
        
        requestAvatar($scope.monologue.from); 

        $scope.isSelf = function() {
            if (typeof $scope.monologue === 'undefined') {
                return false;
            }
            else {
                return (Number($scope.monologue.from) === Number(Origin.user.userPid()));
            }
        };

        $scope.showUsername = function() {
            return ($scope.conversation.state === 'MULTI_USER') && !$scope.isSelf();
        };

        function onAddMessage() {
            $scope.$digest();
        }

        ConversationFactory.events.on('addMessage', onAddMessage);

        function onDestroy() {
            ConversationFactory.events.off('addMessage', onAddMessage);
        }

        $scope.$on('$destroy', onDestroy);

        $scope.getHistoryWidth = function () {
            return $parentHistory[0].clientWidth;
        };

        $scope.$watch($scope.getHistoryWidth, function (newValue, oldValue) {
            var totalWidth = newValue,
                maxWidth = totalWidth - MONOLOGUE_BUBBLE_MAXWIDTH_SUBTRACT;

            oldValue = oldValue;

            $bubbleWidthStyles.text('.origin-social-chatwindow-conversation-historyitem-monologue .origin-social-chatwindow-conversation-historyitem-monologue-bubble { max-width: ' + maxWidth + 'px; } ');

        }, true);
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationHistoryitemMonologue
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} monologue (optional) Javascript object representing the monologue. Not needed for typing indicator use.
     * @param {Object} conversation Javascript object representing the conversation
     * @description
     *
     * origin chatwindow -> conversation -> history item -> monologue
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-historyitem-monologue
     *            conversation="conversation"
     *            monologue="monologue"
     *         ></origin-social-chatwindow-conversation-historyitem-monologue>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationHistoryitemMonologueCtrl', OriginSocialChatwindowConversationHistoryitemMonologueCtrl)
        .directive('originSocialChatwindowConversationHistoryitemMonologue', function(ComponentsConfigFactory) {
        
            return {
                restrict: 'E',
                transclude: true,
                controller: 'OriginSocialChatwindowConversationHistoryitemMonologueCtrl',
                scope: {
                    monologue: '=?',
                    conversation: '='
                },
                link: function(scope, element, attrs, ctrl) { 

                    ctrl.setElement($(element).find('.origin-social-chatwindow-conversation-historyitem-monologue'));

                    // User clicked on an avatar
                    $(element).on('click', '.origin-social-chatwindow-conversation-historyitem-monologue-avatar .otkavatar', function() {
                        ctrl.clickUser();
                    }); 

                    // User clicked on a users name
                    $(element).on('click', '.origin-social-chatwindow-conversation-historyitem-monologue-username', function() {
                        ctrl.clickUser();
                    });

                    // User clicked on a hyperlink in the chat history
                    $(element).on('click', '.otka.user-provided', function(event) {
                        var url = $(this).attr('href');
                        event.stopImmediatePropagation();
                        event.preventDefault();
                        ctrl.clickURL(url);
                    }); 

                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/historyitem/views/monologue.html')
            };
        
        });
}());

