/**
* Created by tirhodes on 2015-02-25.
* @file area.js
*/
(function () {

    'use strict';
    var MIN_TAB_WIDTH = 140;
    var TAB_OVERFLOW_CONTROL_WIDTH = 60;
    var $tabArea;

    /**
    * The controller
    */
    function OriginSocialChatwindowAreaCtrl($scope, $timeout, ChatWindowFactory, ConversationFactory, RosterDataFactory, UtilFactory, SocialDataFactory) {
        var $element,
            activeOneToOnePartnerId = 0;

        $scope.windowTitleStr = '';
        $scope.activeOneToOnePartner = {};
        $scope.overflowUnreadCount = 0;

        function showChatWindow(show) {
            if (show) {
                // need to show the element incase the window was previously popped out
                if (!ChatWindowFactory.isWindowPoppedOut()) {
                    $element.show();
                }
                $('.origin-social-chatwindow-area').show();

                manageOverflowTabs();

                if (ChatWindowFactory.isWindowMinimized()) {
                    ChatWindowFactory.unminimizeWindow();
                }

            } else {
                $('.origin-social-chatwindow-area').hide();
            }
        }

        this.setElement = function($el) {
            $element = $el;
            $tabArea = $element.find('.origin-social-chatwindow-area-tabs');
        };

        this.getTotalNotificationCount = function () {
            return $scope.totalNotificationCount;
        };

        function $getTabByConversationId(conversationId) {
            return $('.origin-social-chatwindow-tab[data-conversation-id="' + conversationId + '"]');
        }

        function $getOverflowMenuItemByConversationId(conversationId) {
            return $('.origin-social-chatwindow-tab-overflow-menu-item[data-conversation-id="' + conversationId + '"]');
        }

        function $getConversationSectionByConversationId(conversationId) {
            return $('.origin-social-chatwindow-conversation-section[data-conversation-id="' + conversationId + '"]');
        }

        function $getActiveTab() {
            return $('.origin-social-chatwindow-tab.otktab.otktab-active');
        }

        function getTabConversationId($tab) {
            return $tab.attr('data-conversation-id');
        }

        function onChatWindowClosed() {
            ConversationFactory.closeAllConversations();
            showChatWindow(false);
        }

        function onSelectTab(id) {
            var $tab = $getTabByConversationId(id);
            if ($tab.length) {
                selectTab($tab);
            }
            updateTitlebar();
        }

        function onOpenConversation(conversationId) {
            // If this is the first conversation we need to raise it instead
            if (conversationCount() === 1) {
                onRaiseConversation(conversationId);
            }
            else {
                // This tab needs to go at the end
                // If we have overflow tabs stick it at the end of the overflowed tabs
                // Otherwise stick it at the end of the visible tabs and call manageOverflowTabs
                // which may end up pushing this tab into the overflow area as the first overflow tab
                // if there isn't room for it
                if (ChatWindowFactory.overflowTabIds().length) {
                    if (ChatWindowFactory.overflowTabIds().indexOf(conversationId) === -1) {
                        // Add it as the last overflow tab if we don't already have a tab for it
                        ChatWindowFactory.overflowTabIds().push(conversationId);
                    }
                }
                else {
                    if (ChatWindowFactory.visibleTabIds().indexOf(conversationId) === -1) {
                        // Add it as the last visible tab if we don't already have a tab for it
                        ChatWindowFactory.visibleTabIds().push(conversationId);
                    }
                }

                manageOverflowTabs();

                $scope.$digest();

                updateTitlebar();
            }
        }

        function onRaiseConversation(conversationId) {
            var $tab;

            if (ChatWindowFactory.visibleTabIds().indexOf(conversationId) === -1) {
                // Add it as the first tab if we don't already have a tab for it
                ChatWindowFactory.visibleTabIds().unshift(conversationId);
            }

            // Bring the tab to the front of the visible tabs if it's currently overflowed
            makeTabVisible(conversationId, true);
            manageOverflowTabs();

            $scope.$digest();

            $tab = $getTabByConversationId(conversationId);
            if ($tab.length) {
                selectTab($tab);
            }

            updateTitlebar();
            showChatWindow(true);
        }

        function onCloseConversation(conversationId) {

            var $closingTab = $getTabByConversationId(conversationId), $sibling,
                            closingTabId = getTabConversationId($closingTab),
                            activeConversationId = getTabConversationId($getActiveTab());

            removeTabFromLists(conversationId);
            manageOverflowTabs();

            if (activeConversationId === closingTabId) {
                // We just closed the active tab

                if (conversationCount() > 0) {
                    if ($closingTab.length) {
                        // Elect a new tab to select
                        // First check for an existing tab to the right
                        $sibling = $closingTab.next('.origin-social-chatwindow-tab');
                        if ($sibling.length) {
                            selectTab($sibling);
                        }
                        else {
                            // No tab to the right, check to the left
                            $sibling = $closingTab.prev('.origin-social-chatwindow-tab');
                            if ($sibling.length) {
                                selectTab($sibling);
                            }
                            else {
                                // Just select the first tab
                                selectTab($('.origin-social-chatwindow-tab.otktab:first'));
                            }
                        }
                    }
                } // else this was the last tab
            }

            updateTitlebar();
            $scope.$digest();
        }

        function animateBadge($badge) {
            // trigger the badge animation
            $badge.removeClass('otkbadge-isnotifying');
            $timeout(function() {
                $badge.addClass('otkbadge-isnotifying');
            },0);
        }

        function onAddMessageToConversation(conversationId) {
            var activeConversationId = getTabConversationId($getActiveTab()),
                $tabBadge = $getTabByConversationId(conversationId).find('.otkbadge'),
                $overflowMenuBadge = $getOverflowMenuItemByConversationId(conversationId).find('.otkbadge');

            // Immediately clear out the unread count if this is the active conversation
            if (!ChatWindowFactory.isWindowMinimized() && (conversationId === activeConversationId)) {
                ConversationFactory.markConversationReadById(conversationId);
            }

            animateBadge($tabBadge);
            animateBadge($overflowMenuBadge);

            updateUnreadCount();
        }

        function updateUnreadCount() {
            var oldCount = $scope.overflowUnreadCount,
                $overflowBadge = $element.find('.origin-social-chatwindow-tab-overflow-tab-unread-badge');

            $scope.overflowUnreadCount = 0;

            $.each(ChatWindowFactory.overflowTabIds(), function (index, value) {
                $scope.overflowUnreadCount += ConversationFactory.getConversationById(value).unreadCount;
            });

            if ($scope.overflowUnreadCount !== oldCount) {
                animateBadge($overflowBadge);
            }

            $scope.$digest();
        }

        function updateTitlebar() {
            var $activeTab, conversation, activeConversationId;

            $activeTab = $getActiveTab();

            if ($activeTab.length) {
                activeConversationId = $activeTab.attr('data-conversation-id');
                conversation = ConversationFactory.getConversationById(activeConversationId);
                if (!!conversation) {
                    // Update the title string
                    $scope.windowTitleStr = $scope.shouldShowTabs() ? '' : conversation.title;

                    // Update the titlebar presence dot
                    $scope.showTitlebarPresence = ((conversation.state === 'ONE_ON_ONE') && (!$scope.shouldShowTabs())) ? true : false;
                }
                else {
                    // Conversation not found
                    // Most likely because it's just been closed
                    $scope.windowTitleStr = '';
                    $scope.showTitlebarPresence = false;
                }
            }
            else {
                // There is no active tab
                // Most likely because all the tabs have been closed
                $scope.windowTitleStr = '';
                $scope.showTitlebarPresence = false;
            }
        }

        function selectTab($tab) {
            var conversationId, $conversationSection, conversation, nucleusId, $activeTab;

            $activeTab = $getActiveTab();

            if ($activeTab.attr('data-conversation-id') !== $tab.attr('data-conversation-id')) {

                // clear out current active tab
                $('.origin-social-chatwindow-tab.otktab-active').removeClass('otktab-active');

                // activate clicked tab
                $tab.addClass('otktab-active');

                // hide current content
                $('.origin-social-chatwindow-conversation-section').hide();

                conversationId = $tab.attr('data-conversation-id');

                // Clear out any notifications
                ConversationFactory.markConversationReadById(conversationId);
                updateUnreadCount();

                // Update the active conversation
                ConversationFactory.setActiveConversationId(conversationId);

                // show active content
                $conversationSection = $getConversationSectionByConversationId(conversationId);

                // Show the active conversation
                $conversationSection.show();

                // Focus the input
                $timeout(function() {
                    var $textInput = $element.find('.origin-social-chatwindow-conversation-input');
                    $textInput.focus();
                },0);

                // Stop listening to the previously selected user
                if (activeOneToOnePartnerId !== 0) {
                    unregisterForPresenceEvent(activeOneToOnePartnerId);
                }

                // Update activeOneToOnePartner
                conversation = ConversationFactory.getConversationById(conversationId);
                if (conversation.state === 'ONE_ON_ONE') {
                    nucleusId = conversation.participants[0];
                    registerForPresenceEvent(nucleusId);
                    RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
                        $scope.activeOneToOnePartner.presence = user.presence;
                        if (!!user.presence) {
                            translatePresence(user.presence);
                        }
                        $scope.$digest();
                    });
                }
            }
        }

        this.onTabClick = function (tab) {
            selectTab($(tab).parents('.otktab'));
        };

        this.onTabOverflowMenuItemClick = function (item) {
            var conversationId = $(item).parents('[data-conversation-id]').attr('data-conversation-id');
            onRaiseConversation(conversationId);
        };

        this.onCloseTabClick = function (tab) {
            var conversationId = $(tab).parents('.otktab').attr('data-conversation-id');
            ConversationFactory.closeConversationById(conversationId);
        };

        this.onCloseOverflowMenuItemClick = function (item) {
            var conversationId = $(item).parents('[data-conversation-id]').attr('data-conversation-id');
            ConversationFactory.closeConversationById(conversationId);
        };

        function translatePresence(presence) {
            $scope.activeOneToOnePartner.joinable = false;
            $scope.activeOneToOnePartner.ingame = false;
            $scope.activeOneToOnePartner.broadcasting = false;
            if (presence.show === 'ONLINE') {
                if (presence.gameActivity && presence.gameActivity.title !== undefined && presence.gameActivity.title !=='') {
                    // If we have a gameActivity object and a title we must be playing a game.
                    $scope.activeOneToOnePartner.ingame = true;
                    if (presence.gameActivity.joinable){
                        $scope.activeOneToOnePartner.joinable = true;
                    }

					// If we have a twitch presence we must be broadcasting
					if (!!presence.gameActivity.twitchPresence && presence.gameActivity.twitchPresence.length) {
						$scope.activeOneToOnePartner.broadcasting = true;
					}
                }
            }
        }

        function handlePresenceChange() {
            var nucleusId = this;
            RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
                var pres = user.presence;
                if (!!pres) {
                    $scope.activeOneToOnePartner.presence = pres;
                    translatePresence(pres);
                    $scope.$digest();
                }
            });
        }

        function registerForPresenceEvent(nucleusId) {
            activeOneToOnePartnerId = nucleusId;
            RosterDataFactory.events.on('socialPresenceChanged:' + nucleusId, handlePresenceChange, nucleusId);
        }

        function unregisterForPresenceEvent(nucleusId) {
            activeOneToOnePartnerId = 0;
            RosterDataFactory.events.off('socialPresenceChanged:' + nucleusId, handlePresenceChange, nucleusId);
        }

        function conversationCount() {
            return Object.keys($scope.conversations).length;
        }

        function manageOverflowTabs() {
            var availableWidth, numTabsThatCanBeShown, numTabsToMove, i, count,
                visibleTabIds = ChatWindowFactory.visibleTabIds(),
                overflowTabIds = ChatWindowFactory.overflowTabIds();

            if (conversationCount() > 1) {
                availableWidth = $tabArea.innerWidth()-TAB_OVERFLOW_CONTROL_WIDTH;
                if (availableWidth > 0) {
                    numTabsThatCanBeShown = Math.floor(availableWidth / MIN_TAB_WIDTH);
                    if (visibleTabIds.length > numTabsThatCanBeShown) {
                        // We have too many visible tabs and need to move some to the overflow list
                        numTabsToMove = visibleTabIds.length - numTabsThatCanBeShown;
                        for (i=visibleTabIds.length-1, count=0; count < numTabsToMove; i--, count++) {
                            makeTabOverflowed(visibleTabIds[i]);
                        }
                    }
                    else if ((visibleTabIds.length < numTabsThatCanBeShown) && overflowTabIds.length) {
                        // We have room to move some of the overflowed tabs to visible
                        numTabsToMove = numTabsThatCanBeShown - visibleTabIds.length;
                        if (numTabsToMove > overflowTabIds.length) {
                            numTabsToMove = overflowTabIds.length;
                        }
                        for (i=0; i < numTabsToMove; i++) {
                            makeTabVisible(overflowTabIds[0], false);
                        }
                    }
                }
            }

            updateUnreadCount();

            $scope.$digest();
        }

        function makeTabVisible(conversationId, moveToFront) {
            var visibleTabIds = ChatWindowFactory.visibleTabIds(),
                overflowTabIds = ChatWindowFactory.overflowTabIds();

            // Remove it from the overflow tabs
            var index = overflowTabIds.indexOf(conversationId);
            if (index > -1) {
                overflowTabIds.splice(index, 1);
            }
            // Make sure it's not already in the list
            if (visibleTabIds.indexOf(conversationId) === -1) {
                if (moveToFront) {
                    // Add it to the front of the visible tabs so it becomes the first tab
                    visibleTabIds.unshift(conversationId);
                }
                else {
                    // Add it to the end of the visible tabs so it becomes the last tab
                    visibleTabIds.push(conversationId);
                }
            }
        }

        function makeTabOverflowed(conversationId) {
            var visibleTabIds = ChatWindowFactory.visibleTabIds(),
                overflowTabIds = ChatWindowFactory.overflowTabIds();

            // Remove it from the visible tabs
            var index = visibleTabIds.indexOf(conversationId);
            if (index > -1) {
                visibleTabIds.splice(index, 1);
            }
            // Make sure it's not already in the list
            if (overflowTabIds.indexOf(conversationId) === -1) {
                // Add it to the front of the overflowed tabs so it becomes the first item in the overflow list
                overflowTabIds.unshift(conversationId);
            }
        }

        function removeTabFromLists(conversationId) {
            var visibleTabIds = ChatWindowFactory.visibleTabIds(),
                overflowTabIds = ChatWindowFactory.overflowTabIds();

            // Remove it from the visible tabs
            var index = visibleTabIds.indexOf(conversationId);
            if (index > -1) {
                visibleTabIds.splice(index, 1);
            }
            // Remove it from the overflow tabs
            index = overflowTabIds.indexOf(conversationId);
            if (index > -1) {
                overflowTabIds.splice(index, 1);
            }
        }

        $scope.conversations = ConversationFactory.getConversations();

        $scope.shouldShowTabs = function () {
            return conversationCount() > 1;
        };

        $scope.getConversationById = function (conversationId) {
            return ConversationFactory.getConversationById(conversationId);
        };

        $scope.getNotificationCountById = function (conversationId) {
            return $scope.notificationCounts[conversationId];
        };

        $scope.getVisibleTabIds = function () {
            return ChatWindowFactory.visibleTabIds();
        };

        $scope.getOverflowTabIds = function () {
            return ChatWindowFactory.overflowTabIds();
        };

        window.removeAllEvents = function() {
            // Need to remove all events from the pop out window,
            // this allows us to create subsequent windows after the first one is closed
            ConversationFactory.events.off('conversationClosed', onCloseConversation);
            ConversationFactory.events.off('addMessage', onAddMessageToConversation);
            ChatWindowFactory.events.off('openConversation', onOpenConversation);
            ChatWindowFactory.events.off('raiseConversation', onRaiseConversation);
            ChatWindowFactory.events.off('closeChatWindow', onChatWindowClosed);
            ChatWindowFactory.events.off('selectTab', onSelectTab);
        };

        ConversationFactory.events.on('conversationClosed', onCloseConversation);
        ConversationFactory.events.on('addMessage', onAddMessageToConversation);

        ChatWindowFactory.events.on('openConversation', onOpenConversation);
        ChatWindowFactory.events.on('raiseConversation', onRaiseConversation);
        ChatWindowFactory.events.on('closeChatWindow', onChatWindowClosed);
        ChatWindowFactory.events.on('selectTab', onSelectTab);

        ChatWindowFactory.events.on('unminimizeChatWindow', function () {
            $element.show();
            ConversationFactory.markConversationReadById(getTabConversationId($getActiveTab()));
            updateUnreadCount();
            $scope.$digest();
        });

        ChatWindowFactory.events.on('minimizeChatWindow', function () {
            $element.hide();
        });

        ChatWindowFactory.events.on('popOutChatWindow', function () {
            console.log('this.popoutSubWindowArea');
            var windowTop = 0;
            var windowLeft = 0;

            if (window) {
                windowTop = window.screenY + $element.offset().top - 10;
                windowLeft = window.screenX + $element.offset().left - 10;
            }
            var popoutChatAreaWindow = null;
            if (typeof OriginIGO !== 'undefined') {
                popoutChatAreaWindow = window.openIGOWindow('socialwindow', '');
            }
            else {
                popoutChatAreaWindow = window.open('social-chatwindow.html', 'socialchat', 'location=0,toolbar=no,menubar=no,dialog=yes,top='+windowTop+',left='+windowLeft+',width='+$tabArea.width+',height='+ $tabArea.height);
            }

            popoutChatAreaWindow.onload = function() {
                // move this to the link code once the post-repeat is merged in
                setTimeout(function() {
                    var id = ConversationFactory.getActiveConversationId();
                    ChatWindowFactory.selectTab(id);
                }, 1500);
            };

            popoutChatAreaWindow.onbeforeunload = function() {
                ChatWindowFactory.closeWindow();
                popoutChatAreaWindow.removeAllEvents();
                $element.show();
            };

            window.onbeforeunload = function() {
                popoutChatAreaWindow.close();
            };
            $element.hide();
        });

        ChatWindowFactory.events.on('unpopOutChatWindow', function () {
            if (!ChatWindowFactory.isWindowMinimized() && !ChatWindowFactory.isWindowPoppedOut()) {
                $element.show();
            } else {
                $element.hide();
            }
        });

		function handleJssdkLogin(login) {
			$scope.userLoggedIn = login;
			$scope.$digest();
		}
		SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);

        $(window).resize( UtilFactory.throttle(function() {
            manageOverflowTabs();
        }, 300));
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowArea
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * origin chatwindow area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-area></origin-social-chatwindow-area>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowAreaCtrl', OriginSocialChatwindowAreaCtrl)
        .directive('originSocialChatwindowArea', function (ComponentsConfigFactory, ChatWindowFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowAreaCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/views/area.html'),

                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    ctrl.setElement($(element));

                    // clicked on a tab
                    $(element).on('click', '.origin-social-chatwindow-tab', function (event) {
                        ctrl.onTabClick(event.target);
                    });

                    // clicked on an overflow menu item
                    $(element).on('click', '.origin-social-chatwindow-tab-overflow-menu-item', function (event) {
                        ctrl.onTabOverflowMenuItemClick(event.target);
                    });

                    // clicked on the close button of a tab
                    $(element).on('click', '.origin-social-chatwindow-tab-close', function (event) {
                        event.stopImmediatePropagation();
                        event.preventDefault();
                        ctrl.onCloseTabClick(event.target);
                    });

                    // clicked on the close button of a tab overflow menu item
                    $(element).on('click', '.origin-social-chatwindow-tab-overflow-menu-item-close', function (event) {
                        event.stopImmediatePropagation();
                        event.preventDefault();
                        ctrl.onCloseOverflowMenuItemClick(event.target);
                    });

                    // Window close button clicked
                    $(element).on('click', '.origin-titlebar-subwindow-close', function () {
                        ChatWindowFactory.closeWindow();
                    });

                    // Window minimize button clicked
                    $(element).on('click', '.origin-titlebar-subwindow-minimize', function() {
                        ChatWindowFactory.minimizeWindow();
                    });

                    // Popout button clicked
                    $(element).on('click', '.origin-titlebar-subwindow-popout', function() {
                        ChatWindowFactory.popOutWindow();
                    });

                    // show the tab overflow menu
                    $(element).on('click', '.otkdropdown-trigger', function (event) {
                        var $dropdownWrap = $(element).find('.otkdropdown .otkdropdown-wrap');
                        $dropdownWrap.toggleClass('otkdropdown-isvisible');
                        event.stopImmediatePropagation();
                        event.preventDefault();
                    });

                    // dismiss the tab overflow menu
                    $(document).on('click', function () {
                        var $dropdownWrap = $(element).find('.otkdropdown .otkdropdown-wrap');
                        $dropdownWrap.removeClass('otkdropdown-isvisible');
                    });

                    if (ChatWindowFactory.isWindowMinimized() || ChatWindowFactory.isWindowPoppedOut()) {
                        if (window.opener) {
                            $(element).show();
                        }
                        else {
                            $(element).hide();
                        }
                    }
                }
            };
        });
} ());
