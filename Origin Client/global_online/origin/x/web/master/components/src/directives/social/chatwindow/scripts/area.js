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
    function OriginSocialChatwindowAreaCtrl($scope, $timeout, ChatWindowFactory, ConversationFactory, RosterDataFactory, UtilFactory, SocialDataFactory, EventsHelperFactory) {
        var $element,
            activeOneToOnePartnerId = 0,
            handles = [],
            popoutChatAreaWindow = null;

        $scope.windowTitleStr = '';
        $scope.activeOneToOnePartner = {};
        $scope.overflowUnreadCount = 0;
        $scope.xmppDisconnected = !Origin.xmpp.isConnected();
        $scope.poppedOut = ChatWindowFactory.isWindowPoppedOut();
        $scope.unpopped = false;
        $scope.igoContext = false;

        // for now assume that if we have extra parameters on the window it's an OIG window
        var windowParams = window.extraParams;
        if (windowParams) {
            $scope.igoContext = true;
            ChatWindowFactory.setOIGContext(true);
        }

        function showChatWindow(show) {
            if (show) {
                // need to show the element incase the window was previously popped out
                if (!$scope.poppedOut) {
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

        function onChatWindowClosed(keepConversations) {
            // We could have popped in the chat window and still want to keep the conversations around
            if (!keepConversations) {
                showChatWindow(false);
                ConversationFactory.closeAllConversations();
                // Need to ensure the popped out window is closed
                // Could be closing from the hub context-menu
                if ($scope.poppedOut) {
                    if (ChatWindowFactory.isOIGWindow()){
                        ChatWindowFactory.setOIGContext(false);
                    }
                    var url = window.url;
                    if (url) {
                        if (window.OriginPopOut) {
                            window.OriginPopOut.popInWindow(url);
                        }
                        else if (window.opener.OriginPopOut) {
                            window.opener.OriginPopOut.popInWindow(url);
                        }
                    }
                    else {
                        window.close();
                    }
                }
            }
            else {
                showChatWindow(true);
            }
        }

        function onSelectTab(id) {
            $timeout(function() {
            console.log('Looking for conversation: ', id);
            var $tab = $getTabByConversationId(id);
            console.log('Found tab: ', $tab);
            if ($tab.length) {
                selectTab($tab);
            }
            updateTitlebar();

            },0);
        }

        function onOpenConversation(conversationId) {
			if (conversationCount() === 1) {
				onRaiseConversation(conversationId);
			}
			else {
				$timeout(function() {
					// If this is the first conversation we need to raise it instead
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
					updateTitlebar();
					
				});			
			}
        }

        function onRaiseConversation(conversationId) {
			$timeout(function() {
				var $tab;

				if (ChatWindowFactory.visibleTabIds().indexOf(conversationId) === -1) {
					// Add it as the first tab if we don't already have a tab for it
					ChatWindowFactory.visibleTabIds().unshift(conversationId);
				}

				// Bring the tab to the front of the visible tabs if it's currently overflowed
				makeTabVisible(conversationId, true);
				manageOverflowTabs();

				$tab = $getTabByConversationId(conversationId);
				if ($tab.length) {
					selectTab($tab);
				}
				if (ChatWindowFactory.isWindowPoppedOut() && !$scope.poppedOut) {
					return;
				}
				updateTitlebar();
				showChatWindow(true);
			});

        }

        function onCloseConversation(conversationId) {
			$timeout(function() {
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
			});			
        }

        function animateBadge($badge) {
            // trigger the badge animation
            $badge.removeClass('otkbadge-isnotifying');
            $timeout(function() {
                $badge.addClass('otkbadge-isnotifying');
            },0);
        }

        function triggerUnreadNotification() {
            var oldCount = $scope.unreadCount;

            $scope.unreadCount = 0;

            // first grab the overflow unread count
            $.each(ChatWindowFactory.overflowTabIds(), function (index, value) {
                $scope.unreadCount += ConversationFactory.getConversationById(value).unreadCount;
            });

            // now grab the visible unread count
            $.each(ChatWindowFactory.visibleTabIds(), function (index, value) {
                $scope.unreadCount += ConversationFactory.getConversationById(value).unreadCount;
            });

            // call into the C++ so we can tigger the taskbar
            if ($scope.unreadCount >= oldCount) {
                Origin.client.desktopServices.flashIcon($scope.unreadCount);
            }

            $scope.$digest();
        }

        function onAddMessageToConversation(conversationId) {
			$timeout(function() {
				var activeConversationId = getTabConversationId($getActiveTab()),
					$tabBadge = $getTabByConversationId(conversationId).find('.otkbadge'),
					$overflowMenuBadge = $getOverflowMenuItemByConversationId(conversationId).find('.otkbadge');

                // Immediately clear out the unread count if this is the active conversation
                if (document.hasFocus() && !ChatWindowFactory.isWindowMinimized() && (conversationId === activeConversationId)) {
                    ConversationFactory.markConversationReadById(conversationId);
                }

                    animateBadge($tabBadge);
                    animateBadge($overflowMenuBadge);

                updateUnreadCount();
            });
        }

        function onIncomingVoiceCall(nucleusId, friend, isNewConversation) {
                // If the chat window is popped out, but this isn't the popped out version return.
                if (ChatWindowFactory.isWindowPoppedOut()) {
                    if (!$scope.poppedOut) {
                        return;
                    }
                }
                // Also early return if this is not an IGO window
                if (ChatWindowFactory.isOIGWindow() && !$scope.igoContext) {
                    return;
                }

                var isChatWindowActive = (document.hasFocus() && !ChatWindowFactory.isWindowMinimized() && nucleusId === ConversationFactory.getActiveConversationId());
                if (isNewConversation || !isChatWindowActive) {
                    Origin.client.desktopServices.flashIcon();
                    Origin.voice.showToast('INCOMING', friend.originId, nucleusId);
                }
        }

        function updateUnreadCount() {
			var oldCount = $scope.overflowUnreadCount,
				$overflowBadge = $element.find('.origin-social-chatwindow-tab-overflow-tab-unread-badge');

			$scope.overflowUnreadCount = 0;

			$.each(ChatWindowFactory.overflowTabIds(), function (index, value) {
				$scope.overflowUnreadCount += ( ConversationFactory.getConversationById(value) ? ConversationFactory.getConversationById(value).unreadCount : 0);
			});

			if ($scope.overflowUnreadCount !== oldCount) {
				animateBadge($overflowBadge);
			}

            // for now this will only trigger in the embedded client
            if (Origin.client.isEmbeddedBrowser()) {
                triggerUnreadNotification();
            }
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
            var conversationId = $tab.attr('data-conversation-id'),
                $activeTab = $getActiveTab(),
                activeConversationId = $activeTab.attr('data-conversation-id'),
                $conversationSection, conversation, nucleusId;

            if (activeConversationId !== conversationId) {

                // clear out current active tab
                $('.origin-social-chatwindow-tab.otktab-active').removeClass('otktab-active');

                // activate clicked tab
                $tab.addClass('otktab-active');

                // hide current content
                $('.origin-social-chatwindow-conversation-section').hide();

                if (activeConversationId !== undefined) {
                    // Clear out any notifications
                    ConversationFactory.markConversationReadById(conversationId);
                    updateUnreadCount();
                }

                // Update the active conversation
                ConversationFactory.setActiveConversationId(conversationId);

                // show active content
                $conversationSection = $getConversationSectionByConversationId(conversationId);

                // Show the active conversation
                $conversationSection.show();

                // Focus the input, will call a digest cycle
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

        function handlePresenceChange(presence) {
            $scope.activeOneToOnePartner.presence = presence;
            translatePresence(presence);
            $scope.$digest();
        }

        function registerForPresenceEvent(nucleusId) {
            activeOneToOnePartnerId = nucleusId;
            RosterDataFactory.events.on('socialPresenceChanged:' + nucleusId, handlePresenceChange);
        }

        function unregisterForPresenceEvent(nucleusId) {
            activeOneToOnePartnerId = 0;
            RosterDataFactory.events.off('socialPresenceChanged:' + nucleusId, handlePresenceChange);
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

        function onUnminimizeChatWindow() {
			$timeout(function() {
				$element.show();
				ConversationFactory.markConversationReadById(getTabConversationId($getActiveTab()));
				updateUnreadCount();
			});
        }

        function onMinimizeChatWindow() {
            $element.hide();
        }

        function onPopOutChatWindow() {
            console.log('this.popoutSubWindowArea');
            var size = {
                'top' : window.screenY + $element.offset().top - 10,
                'left' : window.screenX + $element.offset().left - 10,
                'width' : $tabArea.width,
                'height' : $tabArea.height
            };
            popoutChatAreaWindow = window._openPopOutWindow('socialwindow', '', size);

             if (window.OriginPopOut) {
                window.OriginPopOut.popOutClosed.connect(function(url) {
                    // C++ closed this window
                    if (popoutChatAreaWindow.url === url) {
                        popoutChatAreaWindow.close();
                    }
                });
            }

            window.onbeforeunload = function() {
                popoutChatAreaWindow.close();
            };
            $element.hide();
        }


        function onUnpopOutChatWindow() {
            if (!ChatWindowFactory.isWindowMinimized() && !$scope.poppedOut) {
                $element.show();
            } else {
                $scope.unpopped = true;
                var url = window.url;
                if (url) {
                    if (window.OriginPopOut) {
                        window.OriginPopOut.popInWindow(url);
                    }
                    else if (window.opener.OriginPopOut) {
                        window.opener.OriginPopOut.popInWindow(url);
                    }
                }
                else {
                    window.close();
                }
                popoutChatAreaWindow = null;
            }
        }

        function onXmppConnect() {
            $scope.xmppDisconnected = false;
            $scope.$digest();
        }

        function onXmppDisconnect() {
            $scope.xmppDisconnected = true;
            $scope.$digest();
        }
 
        function onDesktopDockIconClicked() {
            if (ChatWindowFactory.isWindowPoppedOut()) {
                window._moveWindowToFront(popoutChatAreaWindow);
            }
        }

        // listen for xmpp connect/disconnect
        RosterDataFactory.events.on('xmppConnected', onXmppConnect);
        RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);

        function onDestroy() {
            SocialDataFactory.events.off('jssdkLogin', handleJssdkLogin);
            RosterDataFactory.events.off('xmppConnected', onXmppConnect);
            RosterDataFactory.events.off('xmppDisconnected', onXmppDisconnect);
        }
        $scope.$on('$destroy', onDestroy);

		function handleJssdkLogin(login) {
            $scope.userLoggedIn = login;
		}
		handles[handles.length] = SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);

        function showActiveConversation(e) {
            SocialDataFactory.updateLoginStatus();
            var id = ConversationFactory.getActiveConversationId();
            console.log("Active Conversation should be: ", id);
            if (id !== undefined){
                ChatWindowFactory.selectTab(id);
            }
            e.stopPropagation();
            return false;
        }

        function destroy() {
            ChatWindowFactory.closeWindow($scope.unpopped);
            EventsHelperFactory.detachAll(handles);
        }

        window.addEventListener('unload', destroy);

        handles[handles.length] = ConversationFactory.events.on('conversationClosed', onCloseConversation);
        handles[handles.length] = ConversationFactory.events.on('addMessage', onAddMessageToConversation);
        handles[handles.length] = ConversationFactory.events.on('incomingVoiceCall', onIncomingVoiceCall);
        
        handles[handles.length] = ChatWindowFactory.events.on('openConversation', onOpenConversation);
        handles[handles.length] = ChatWindowFactory.events.on('raiseConversation', onRaiseConversation);
        handles[handles.length] = ChatWindowFactory.events.on('closeChatWindow', onChatWindowClosed);
        handles[handles.length] = ChatWindowFactory.events.on('selectTab', onSelectTab);
        handles[handles.length] = ChatWindowFactory.events.on('unminimizeChatWindow', onUnminimizeChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('unpopOutChatWindow', onUnpopOutChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('minimizeChatWindow', onMinimizeChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('popOutChatWindow', onPopOutChatWindow);
        // listen for xmpp connect/disconnect
        handles[handles.length] = RosterDataFactory.events.on('xmppConnected', onXmppConnect);
        handles[handles.length] = RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);
        // listen for postrepeat so popped out window knows which conversation to load.
        handles[handles.length] = $scope.$on('postrepeat:last', showActiveConversation);
        // listen for desktop events
        handles[handles.length] = Origin.events.on(Origin.events.CLIENT_DESKTOP_DOCKICONCLICKED, onDesktopDockIconClicked);

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

                    // Popin button clicked
                    $(element).on('click', '.origin-titlebar-subwindow-popin', function() {
                        ChatWindowFactory.unpopOutWindow();
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
