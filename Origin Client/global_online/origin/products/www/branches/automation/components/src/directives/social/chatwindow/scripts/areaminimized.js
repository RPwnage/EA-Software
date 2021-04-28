/**
* @file areaminimized.js
*/
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowAreaminimizedCtrl($scope, $timeout, ChatWindowFactory, ConversationFactory, RosterDataFactory, SocialDataFactory, EventsHelperFactory, ObserverFactory) {
        var $element,
            handles = [];

		var observableParticipant;
		
        $scope.activeConversation = {};
        $scope.unreadCount = 0;
        $scope.overflowTabsUnreadCount = 0;

        this.setElement = function($el) {
            $element = $el;
        };

        this.onTabOverflowMenuItemClick = function (item) {
            var conversationId = $(item).parents('[data-conversation-id]').attr('data-conversation-id');
            ChatWindowFactory.raiseConversation(conversationId);
        };

        this.onCloseOverflowMenuItemClick = function (item) {
            var conversationId = $(item).parents('[data-conversation-id]').attr('data-conversation-id');
            ConversationFactory.closeConversationById(conversationId);
        };

        function updateUnreadCounts() {
            var count = 0, conversations = ConversationFactory.getConversations();
            for (var id in conversations) {
                // parseInt both variables so that the triple check only fails when it's supposed to
                if (parseInt(id) !== parseInt($scope.activeConversation.id)) {
                    count += ( ConversationFactory.getConversationById(id) ? ConversationFactory.getConversationById(id).unreadCount : 0);
                }
            }
            $scope.overflowTabsUnreadCount = count;

            $scope.unreadCount = $scope.activeConversation.unreadCount;
        }

        function onUnminimizeChatWindow() {
            updateUnreadCounts();
            $element.hide();
        }

        function onMinimizeChatWindow() {
            updateUnreadCounts();
            $element.show();
        }

        function onUnpopOutChatWindow() {
            if (ChatWindowFactory.isWindowMinimized() && !ChatWindowFactory.isWindowPoppedOut()) {
                $element.show();
            } else {
                $element.hide();
            }
        }

        function animateBadge($badge) {
            // trigger the badge animation
            $badge.removeClass('otkbadge-isnotifying');
            $timeout(function() {
                $badge.addClass('otkbadge-isnotifying');
            },0);
        }

        function onAddMessage(conversationId) {
            var $activeBadge = $('.origin-social-chatwindow-area-minimized-button').find('.otkbadge'),
                $tabSelectorBadge = $('.origin-social-chatwindow-area-minimized-tabselector').find('.otkbadge');

            updateUnreadCounts();

            if (conversationId === ConversationFactory.getActiveConversationId()) {
                animateBadge($activeBadge);
            }
            else {
                animateBadge($tabSelectorBadge);
            }
        }

        function onActiveConversationIdChanged() {
			$timeout(function() {
				var nucleusId;

				$scope.activeConversation = ConversationFactory.getConversationById(ConversationFactory.getActiveConversationId());				
				
				if ($scope.activeConversation.state === 'ONE_ON_ONE') {
					nucleusId = $scope.activeConversation.participants[0];
					
					// Stop listening to the previously selected user
					if (!!observableParticipant) {
						observableParticipant.detach($scope);
					}

					RosterDataFactory.getRosterUser(nucleusId).then(function(user) {
						if (!!user) {
							observableParticipant = 
								ObserverFactory.create(user._presence)
								.defaultTo({jid: nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
								.attachToScope($scope, 'activePresence');									
						} 
					});							
				}

				updateUnreadCounts();
			});			
        }


        $scope.additionalTabCount = function () {
            return ConversationFactory.getNumConversations() - 1;
        };

        $scope.getAdditionalTabIds = function () {

            var idArray = [];

            // Concatenate the visible and overflowed tabs
            idArray = idArray.concat(ChatWindowFactory.visibleTabIds());
            idArray = idArray.concat(ChatWindowFactory.overflowTabIds());

            // Remove the active conversation from the list
            var index = idArray.indexOf(ConversationFactory.getActiveConversationId());
            if (index > -1) {
                idArray.splice(index, 1);
            }

            return idArray;
        };
		
        $scope.getConversationById = function (conversationId) {
            return ConversationFactory.getConversationById(conversationId);
        };		

		function handleJssdkLogin(login) {
			$scope.userLoggedIn = login;
		}

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }

        $scope.$on('$destroy', destroy);

		handles[handles.length] = SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);
        handles[handles.length] = ChatWindowFactory.events.on('unminimizeChatWindow', onUnminimizeChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('minimizeChatWindow', onMinimizeChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('popOutChatWindow', onUnminimizeChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('unpopOutChatWindow', onUnpopOutChatWindow);
        handles[handles.length] = ConversationFactory.events.on('addMessage', onAddMessage);
        handles[handles.length] = ConversationFactory.events.on('activeConversationIdChanged', onActiveConversationIdChanged);

        // listen for xmpp connect/disconnect
		ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
			.defaultTo({isConnected: Origin.xmpp.isConnected()})
			.attachToScope($scope, 'xmppConnection');
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowAreaminimized
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * origin chatwindow minimized area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-areaminimized></origin-social-chatwindow-areaminimized>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowAreaminimizedCtrl', OriginSocialChatwindowAreaminimizedCtrl)
        .directive('originSocialChatwindowAreaminimized', function(ComponentsConfigFactory, ChatWindowFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowAreaminimizedCtrl',
				scope: {},				
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/views/areaminimized.html'),
                link: function(scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    ctrl.setElement($(element));

                    $(element).find('.origin-social-chatwindow-area-minimized-button').on('click', function() {
                        ChatWindowFactory.unminimizeWindow();
                    });

                    // clicked on an overflow menu item
                    $(element).on('click', '.origin-social-chatwindow-tab-overflow-menu-item', function (event) {
                        ctrl.onTabOverflowMenuItemClick(event.target);
                    });

                    // clicked on the close button of a tab overflow menu item
                    $(element).on('click', '.origin-social-chatwindow-tab-overflow-menu-item-close', function (event) {
                        event.stopImmediatePropagation();
                        event.preventDefault();
                        ctrl.onCloseOverflowMenuItemClick(event.target);
                    });

                    // show the tab overflow menu
                    $(element).on('click', '.otkdropdown-trigger', function (event) {
                        var $dropdownWrap = $(element).find('.origin-social-chatwindow-area-minimized-tabselector .otkdropdown-wrap');
                        $dropdownWrap.toggleClass('otkdropdown-isvisible');
                        event.stopImmediatePropagation();
                        event.preventDefault();
                    });

                    // dismiss the tab overflow menu
                    $(document).on('click', function () {
                        var $dropdownWrap = $(element).find('.origin-social-chatwindow-area-minimized-tabselector .otkdropdown-wrap');
                        $dropdownWrap.removeClass('otkdropdown-isvisible');
                    });

                    if (ChatWindowFactory.isWindowMinimized() && !ChatWindowFactory.isWindowPoppedOut()) {
                        $(element).show();
                    } else {
                        $(element).hide();
                    }
                }
            };

        });

} ());
