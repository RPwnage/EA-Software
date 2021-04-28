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
    function OriginSocialChatwindowAreaCtrl($scope, $timeout, $sce, ChatWindowFactory, ConversationFactory, RosterDataFactory, UtilFactory, SocialDataFactory, EventsHelperFactory, ObserverFactory, ComponentsConfigHelper) {
        var $element,
            handles = [],
            isPopOutChatWindowFocused = false,
            detachHandle;

        var observableParticipant;
        $scope.windowTitleStr = '';
        $scope.overflowUnreadCount = 0;
        $scope.poppedOut = ChatWindowFactory.isWindowPoppedOut();
        $scope.unpopped = false;
        $scope.igoContext = ComponentsConfigHelper.isOIGContext();
        ChatWindowFactory.playingGames = [];

        function onfocus() {
            markActiveConversationRead();
        }

        function showChatWindow(show) {
            var chatwindowArea = angular.element('.origin-social-chatwindow-area');
            if (show) {
                // need to show the element incase the window was previously popped out
                if (!$scope.poppedOut) {
                    $element.show();
                }
                chatwindowArea.show();
                window.addEventListener('focus', onfocus);

                manageOverflowTabs();

                if (ChatWindowFactory.isWindowMinimized()) {
                    ChatWindowFactory.unminimizeWindow();
                }
                // Make sure we focus the window if it's popped out already
                onDesktopDockIconClicked();

            } else {
                window.removeEventListener('focus', onfocus);
                angular.element('.origin-social-chatwindow-area').hide();
            }
        }

        this.setElement = function($el) {
            $element = $el;
            $tabArea = $element.find('.origin-social-chatwindow-area-tabs');
        };

        this.getTotalNotificationCount = function () {
            return $scope.totalNotificationCount;
        };

        function getTabByConversationId(conversationId) {
            return angular.element('.origin-social-chatwindow-tab[data-conversation-id="' + conversationId + '"]');
        }

        function $getOverflowMenuItemByConversationId(conversationId) {
            return angular.element('.origin-social-chatwindow-tab-overflow-menu-item[data-conversation-id="' + conversationId + '"]');
        }

        function $getConversationSectionByConversationId(conversationId) {
            return angular.element('.origin-social-chatwindow-conversation-section[data-conversation-id="' + conversationId + '"]');
        }

        function getActiveTab() {
            return angular.element('.origin-social-chatwindow-tab.otktab.otktab-active');
        }

        function getTabConversationId($tab) {
            return $tab.attr('data-conversation-id');
        }

        function onChatWindowClosed(keepConversations) {
            // We could have popped in the chat window and still want to keep the conversations around
            if (!keepConversations) {
                showChatWindow(false);
                ConversationFactory.closeAllConversations();
                // Need to ensure that we close the OIG window
                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.client.oig.closeIGOConversation();
                }
                if ($scope.igoContext) {
                    window.close();
                }
                // Need to ensure the popped out window is closed
                // Could be closing from the hub context-menu
                if ($scope.poppedOut) {
                    var url = window.url;
                    if (url) {
                        if (Origin.client.isEmbeddedBrowser()) {
                            Origin.client.popout.popInWindow(url);
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
                var $tab = getTabByConversationId(id);
                if ($tab.length) {
                    selectTab($tab);
                }
                updateTitlebar();

            },0);
        }

        function onOpenConversation(conversationId) {
            if (conversationCount() === 1) {
                this.onRaiseConversation(conversationId);
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

        this.onRaiseConversation = function(conversationId) {
            $timeout(function() {
                var $tab;

                if (ChatWindowFactory.visibleTabIds().indexOf(conversationId) === -1) {
                    // Add it as the first tab if we don't already have a tab for it
                    ChatWindowFactory.visibleTabIds().unshift(conversationId);
                }

                // Bring the tab to the front of the visible tabs if it's currently overflowed
                makeTabVisible(conversationId, true);
                manageOverflowTabs();

                $tab = getTabByConversationId(conversationId);
                if ($tab.length) {
                    selectTab($tab);
                }
                if (ChatWindowFactory.isWindowPoppedOut() && !$scope.poppedOut) {
                    return;
                }
                updateTitlebar();
                showChatWindow(true);
            });
        };

        this.getActiveConversationId = function() {
            return getTabConversationId(getActiveTab());
        };

         function onCloseConversation(conversationId) {
            $timeout(function() {
                var $closingTab = getTabByConversationId(conversationId), $sibling,
                                closingTabId = getTabConversationId($closingTab),
                                activeConversationId = getTabConversationId(getActiveTab());

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
                var unreadCount = ConversationFactory.getConversationById(value).unreadCount;
                $scope.unreadCount += unreadCount;
            });

            // now grab the visible unread count
            $.each(ChatWindowFactory.visibleTabIds(), function (index, value) {
                var unreadCount = ConversationFactory.getConversationById(value).unreadCount;
                $scope.unreadCount += unreadCount;
            });

            // call into the C++ so we can tigger the taskbar
            // on PC, we only flash if we've received new messages, on Mac, we flash whenever the unread message count changes
            if (Origin.utils.os() === 'MAC') {
                if ($scope.unreadCount !== oldCount) {
                    Origin.client.desktopServices.flashIcon($scope.unreadCount);
                }
            } else {
                if ($scope.unreadCount > oldCount) {
                    Origin.client.desktopServices.flashIcon($scope.unreadCount);
                }
            }

            $scope.$digest();
        }

        function onAddMessageToConversation(conversationId) {
            $timeout(function() {
                var activeTab = getActiveTab(),
                    activeConversationId = getTabConversationId(activeTab),
                    $tabBadge = getTabByConversationId(conversationId).find('.otkbadge'),
                    $overflowMenuBadge = $getOverflowMenuItemByConversationId(conversationId).find('.otkbadge'),
                        isChatWindowPoppedOutAndFocused = (ChatWindowFactory.poppedOutChatWindow !== null && ChatWindowFactory.poppedOutChatWindow.hasFocus()),
                    isChatWindowDockedAndFocused = (ChatWindowFactory.poppedOutChatWindow === null && document.hasFocus()),
                    isActiveTab = conversationId === activeConversationId,
                    isNoActiveTab = activeTab.length === 0;

                // Immediately clear out the unread count if this is the active conversation
                if ((isChatWindowPoppedOutAndFocused || isChatWindowDockedAndFocused) && !ChatWindowFactory.isWindowMinimized() && (isActiveTab || isNoActiveTab)) {
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

                // Return if this is an IGO window but IGO is not in the foreground and vice versa.
                if (($scope.igoContext && !Origin.client.oig.IGOIsActive()) ||
                    (!$scope.igoContext && Origin.client.oig.IGOIsActive())) {
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
            // but we do not want this signal sent from the OIG context
            if (Origin.client.isEmbeddedBrowser() && !$scope.igoContext) {
                triggerUnreadNotification();
            }
        }

        function updateTitlebar() {
            var $activeTab, conversation, activeConversationId;

            $activeTab = getActiveTab();

            if ($activeTab.length) {
                activeConversationId = $activeTab.attr('data-conversation-id');
                conversation = ConversationFactory.getConversationById(activeConversationId);
                if (!!conversation) {
                    // Update the title string
                    $scope.windowTitleStr = $scope.shouldShowTabs() ? '' : $sce.trustAsHtml(conversation.title);

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
                $activeTab = getActiveTab(),
                activeConversationId = $activeTab.attr('data-conversation-id'),
                $conversationSection, conversation, nucleusId;

            if (activeConversationId !== conversationId) {

                // clear out current active tab
                angular.element('.origin-social-chatwindow-tab.otktab-active').removeClass('otktab-active');

                // activate clicked tab
                $tab.addClass('otktab-active');

                // hide current content
                angular.element('.origin-social-chatwindow-conversation-section').hide();

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

                // Update participant observer
                conversation = ConversationFactory.getConversationById(conversationId);
                if (conversation.state === 'ONE_ON_ONE') {
                    nucleusId = conversation.participants[0];
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
            }
        }

        this.onTabClick = function (tab) {
            var tabElement = angular.element(tab);
            if (!!tabElement.attr('data-conversation-id')) {
                selectTab(tabElement);
            }
            else {
                selectTab(tabElement.parents('.otktab'));
            }
        };

        this.onTabOverflowMenuItemClick = function (item) {
            var conversationId = angular.element(item).parents('[data-conversation-id]').attr('data-conversation-id');
            this.onRaiseConversation(conversationId);
        };

        this.onCloseTabClick = function (tab) {
            //the tab could be the actual element tab or the child.
            var conversationId = angular.element(tab).attr('data-conversation-id') || angular.element(tab).parents('.otktab').attr('data-conversation-id');
            ConversationFactory.closeConversationById(conversationId);
        };

        this.onCloseOverflowMenuItemClick = function (item) {
            var conversationId = angular.element(item).parents('[data-conversation-id]').attr('data-conversation-id');
            ConversationFactory.closeConversationById(conversationId);
        };

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

        function markActiveConversationRead() {
            var activeConversationId = getTabConversationId(getActiveTab());
            ConversationFactory.markConversationReadById(activeConversationId);
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
                ConversationFactory.markConversationReadById(getTabConversationId(getActiveTab()));
                $element.find('.origin-social-chatwindow-conversation-input').focus();
                updateUnreadCount();
            });
        }

        function onMinimizeChatWindow() {
            if (!ComponentsConfigHelper.isOIGContext()) {
                $element.hide();
            }
        }

        function onPopOutChatWindow() {
            var isOIG = angular.isDefined(ComponentsConfigHelper.getOIGContextConfig()),
                windowUuid = UtilFactory.guid();
            if (Origin.client.isEmbeddedBrowser()) {
                    Origin.client.desktopServices.setNextWindowUUID(windowUuid);
            }
            if (isOIG) {
                return;
            }
            
            var size = {
                'top' : window.screenY + $element.offset().top - 10,
                'left' : window.screenX + $element.offset().left - 10,
                'width' : $tabArea.width,
                'height' : $tabArea.height
            };
            
            if (!_.isFunction(window._openPopOutWindow)) {
                return;
            }
            
            window._openPopOutWindow('socialwindow', '', '', size).then(function(childWindow){
                ChatWindowFactory.poppedOutChatWindow = childWindow;
            
                ChatWindowFactory.poppedOutChatWindow.OriginWindowUUID = windowUuid;

                 if (Origin.client.isEmbeddedBrowser()) {

                    Origin.events.on(Origin.events.CLIENT_POP_OUT_CLOSED, function(url) {
                        // C++ closed this window
                        if (ChatWindowFactory.poppedOutChatWindow && ChatWindowFactory.poppedOutChatWindow.url === url) {
                            ChatWindowFactory.poppedOutChatWindow.close();
                            ChatWindowFactory.poppedOutChatWindow = null;
                            isPopOutChatWindowFocused = false;
                        }
                    });
                }

                window.onunload = function() {
                    if(window.location.href !== "about:blank") {
                        ChatWindowFactory.poppedOutChatWindow.close();
                        ChatWindowFactory.poppedOutChatWindow = null;
                        isPopOutChatWindowFocused = false;
                    }
                };
                $element.hide();

                ChatWindowFactory.poppedOutChatWindow.onunload = function() {
                    if(ChatWindowFactory.poppedOutChatWindow.location.href !== "about:blank") {
                        ChatWindowFactory.poppedOutChatWindow.close();
                        ChatWindowFactory.poppedOutChatWindow = null;
                        isPopOutChatWindowFocused = false;
                    }
                };


                ChatWindowFactory.poppedOutChatWindow.onfocus = function() {
                    markActiveConversationRead();
                    isPopOutChatWindowFocused = true;
                };

                ChatWindowFactory.poppedOutChatWindow.onblur = function() {
                    isPopOutChatWindowFocused = false;
                };

                ChatWindowFactory.poppedOutChatWindow.onload = function() {
                    ChatWindowFactory.poppedOutChatWindow.focus();
                };

                ChatWindowFactory.poppedOutChatWindow.hasFocus = function() {
                    return ((ChatWindowFactory.poppedOutChatWindow !== null) && isPopOutChatWindowFocused);
                };

                isPopOutChatWindowFocused = true;
            });
        }

        function onMessageReceived(msg) {
            // If the chat window is popped out, but this isn't the popped out version return.
            if (ChatWindowFactory.isWindowPoppedOut()) {
                if (!$scope.poppedOut) {
                    return;
                }
            }

            // Return if this is an IGO window but IGO is not in the foreground and vice versa.
            if (($scope.igoContext && !Origin.client.oig.IGOIsActive()) ||
                (!$scope.igoContext && Origin.client.oig.IGOIsActive())) {
                return;
            }

            var nucleusId = msg.from.split('@')[0];
            var isChatWindowActive = (document.hasFocus() && !ChatWindowFactory.isWindowMinimized() && nucleusId === ConversationFactory.getActiveConversationId());
            if (!isChatWindowActive) {
                Origin.client.social.showChatToast(msg.from, msg.msgBody.replace(/<[^>]*>/g, ''));
            }
        }

        function onUnpopOutChatWindow() {
            if (!ChatWindowFactory.isWindowMinimized() && !ChatWindowFactory.isWindowPoppedOut() && !$scope.poppedOut) {
                $element.show();
            } else {
                $scope.unpopped = true;
                var url = window.url;
                if (url) {
                    if (Origin.client.isEmbeddedBrowser()) {
                        Origin.client.popout.popInWindow(url);
                    }else {
                        window.close();
                    }
                }else {
                    // if this is not a popped out window just call show
                    $element.show();
                }
                ChatWindowFactory.poppedOutChatWindow = null;
                isPopOutChatWindowFocused = false;
            }
        }

        function onDesktopDockIconClicked() {
            if ($scope.igoContext === false) {
                if (!!window && !!ChatWindowFactory.poppedOutChatWindow && ChatWindowFactory.isWindowPoppedOut()) {
                    if (window.hasOwnProperty('_moveWindowToFront') && typeof window._moveWindowToFront === 'function') {
                        window._moveWindowToFront(ChatWindowFactory.poppedOutChatWindow);
                        // This may be getting called from the popped out window itself which does not have the _moveWindowToFront function
                    }else if (window.opener.hasOwnProperty('_moveWindowToFront') && typeof window.opener._moveWindowToFront === 'function') {
                        window.opener._moveWindowToFront(ChatWindowFactory.poppedOutChatWindow);
                    }
                }
            }
        }

        function onShowChatWindowForFriend(friend) {
            if ($scope.igoContext === false) {
                if (!!window) {
                    var conversationId = friend.jid.split('@')[0];
                    ConversationFactory.openConversation(conversationId);

                    if (!!ChatWindowFactory.poppedOutChatWindow && ChatWindowFactory.isWindowPoppedOut()) {
                        window._moveWindowToFront(ChatWindowFactory.poppedOutChatWindow);
                        Origin.client.desktopServices.showWindow(ChatWindowFactory.poppedOutChatWindow.OriginWindowUUID);
                    } else {
                        if (ChatWindowFactory.isWindowMinimized()) {
                            ChatWindowFactory.unminimizeWindow();
                        }
                        window._moveWindowToFront(window);
                        Origin.client.desktopServices.showWindow(window.OriginWindowUUID);
                    }
                }
            }
        }
        
        function onGameStatusChanged(gameInfoObj) {
            var gameInfo = gameInfoObj.updatedGames;

            var isOig = angular.isDefined(ComponentsConfigHelper.getOIGContextConfig());
            if (isOig) {
                return;
            }
            
            if (!ChatWindowFactory.isWindowPoppedOut()) {
                return;
            }
            
            var where = ChatWindowFactory.playingGames.findIndex(function(element /* , index, array */) { return element.productId === gameInfo[0].productId; });
            if (gameInfo[0].playing && where === -1) {
                // an untracked game has started playing
                if (gameInfo[0].isIGOEnabled) {
                    Origin.client.desktopServices.miniaturize(ChatWindowFactory.poppedOutChatWindow.OriginWindowUUID);
                }
                ChatWindowFactory.playingGames.push(gameInfo[0]);
            } else if (!gameInfo[0].playing && where !== -1) {
                // a tracked game has stopped playing
                ChatWindowFactory.playingGames.splice(where, 1);
                
                // determine if an IGO-enabled game is still playing
                var isIgoEnabledGameStillPlaying = false;
                for (var i = 0; i < ChatWindowFactory.playingGames.length; ++i) {
                    isIgoEnabledGameStillPlaying = isIgoEnabledGameStillPlaying || ChatWindowFactory.playingGames[i].isIGOEnabled;
                }
                
                // decloak if that was the last IGO
                if (!isIgoEnabledGameStillPlaying) {
                    Origin.client.desktopServices.deminiaturize(ChatWindowFactory.poppedOutChatWindow.OriginWindowUUID);
                }
            }
        }

        function onConversationRead(id) {
            id = id;
            updateUnreadCount();
        }

        // listen for xmpp connect/disconnect
        ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
            .defaultTo({isConnected: Origin.xmpp.isConnected()})
            .attachToScope($scope, 'xmppConnection');

        function handleJssdkLogin(login) {
            $scope.userLoggedIn = login;
        }
        handles[handles.length] = SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);

        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }
        $scope.$on('$destroy', onDestroy);

        function showActiveConversation(e) {
            SocialDataFactory.updateLoginStatus();
            var id = ConversationFactory.getActiveConversationId();
            if (id !== undefined){
                ChatWindowFactory.selectTab(id);
            }
            e.stopPropagation();
            return false;
        }

        function destroy() {
            ChatWindowFactory.closeWindow($scope.unpopped);
            EventsHelperFactory.detachAll(handles);
            if (detachHandle) {
                detachHandle();
            }
        }

        window.addEventListener('unload', destroy);

        handles[handles.length] = ConversationFactory.events.on('conversationClosed', onCloseConversation);
        handles[handles.length] = ConversationFactory.events.on('addMessage', onAddMessageToConversation);
        handles[handles.length] = ConversationFactory.events.on('incomingVoiceCall', onIncomingVoiceCall);
        handles[handles.length] = ConversationFactory.events.on('conversationRead', onConversationRead);
        handles[handles.length] = ChatWindowFactory.events.on('openConversation', onOpenConversation);
        handles[handles.length] = ChatWindowFactory.events.on('raiseConversation', this.onRaiseConversation);
        handles[handles.length] = ChatWindowFactory.events.on('closeChatWindow', onChatWindowClosed);
        handles[handles.length] = ChatWindowFactory.events.on('selectTab', onSelectTab);
        handles[handles.length] = ChatWindowFactory.events.on('unminimizeChatWindow', onUnminimizeChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('unpopOutChatWindow', onUnpopOutChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('minimizeChatWindow', onMinimizeChatWindow);
        handles[handles.length] = ChatWindowFactory.events.on('popOutChatWindow', onPopOutChatWindow);
        handles[handles.length] = Origin.events.on(Origin.events.CLIENT_SOCIAL_MESSAGERECEIVED, onMessageReceived);

        // listen for postrepeat so popped out window knows which conversation to load.

        detachHandle = $scope.$on('postrepeat:last', showActiveConversation);

        // listen for desktop events
        handles[handles.length] = Origin.events.on(Origin.events.CLIENT_DESKTOP_DOCKICONCLICKED, onDesktopDockIconClicked);
        // listen for chat window related events
        handles[handles.length] = Origin.events.on(Origin.events.CLIENT_SOCIAL_SHOWCHATWINDOWFORFRIEND, onShowChatWindowForFriend);
        // listen for changes in game status (e.g. started playing, stopped playing)
        handles[handles.length] = Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, onGameStatusChanged);

        angular.element(window).resize( UtilFactory.throttle(function() {
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
        .directive('originSocialChatwindowArea', function (ComponentsConfigFactory, ChatWindowFactory, $timeout, KeyEventsHelper) {

            return {
                restrict: 'E',
                scope: {},
                controller: 'OriginSocialChatwindowAreaCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/views/area.html'),
                link: function (scope, element, attrs, ctrl) {
                    attrs = attrs;

                    function onFocusOnActiveChatWindow() {
                        if (ChatWindowFactory.isWindowMinimized()) {
                            ChatWindowFactory.unminimizeWindow();
                        }

                        // Focus the input, will call a digest cycle
                        $timeout(function() {
                            element.find('.origin-social-chatwindow-conversation-input').focus();
                        },0);
                    }

                    ctrl.setElement(element);

                    // clicked on a tab
                    element.on('click', '.origin-social-chatwindow-tab', function (event) {
                        ctrl.onTabClick(event.target);
                    });

                    // clicked on an overflow menu item
                    element.on('click', '.origin-social-chatwindow-tab-overflow-menu-item', function (event) {
                        ctrl.onTabOverflowMenuItemClick(event.target);
                    });

                    // clicked on the close button of a tab
                    element.on('click', '.origin-social-chatwindow-tab-close', function (event) {
                        event.stopImmediatePropagation();
                        event.preventDefault();
                        ctrl.onCloseTabClick(event.target);
                    });

                    // clicked on the close button of a tab overflow menu item
                    element.on('click', '.origin-social-chatwindow-tab-overflow-menu-item-close', function (event) {
                        event.stopImmediatePropagation();
                        event.preventDefault();
                        ctrl.onCloseOverflowMenuItemClick(event.target);
                    });

                    // Window close button clicked
                    element.on('click', '.origin-titlebar-subwindow-close', function () {
                        ChatWindowFactory.closeWindow();
                    });

                    // Window minimize button clicked
                    element.on('click', '.origin-titlebar-subwindow-minimize', function() {
                        ChatWindowFactory.minimizeWindow();
                    });

                    // Popout button clicked
                    element.on('click', '.origin-titlebar-subwindow-popout', function() {
                        ChatWindowFactory.popOutWindow();
                    });

                    // Popin button clicked
                    element.on('click', '.origin-titlebar-subwindow-popin', function() {
                        ChatWindowFactory.unpopOutWindow();
                    });

                    // show the tab overflow menu
                    element.on('click', '.origin-social-chatwindow-tab-overflow-tab', function (event) {
                        var $dropdownWrap = element.find('.otkdropdown .otkdropdown-wrap');
                        $dropdownWrap.toggleClass('otkdropdown-isvisible');
                        event.stopImmediatePropagation();
                        event.preventDefault();
                    });

                    // dismiss the tab overflow menu
                    angular.element(document).on('click', function () {
                        var $dropdownWrap = element.find('.otkdropdown .otkdropdown-wrap');
                        $dropdownWrap.removeClass('otkdropdown-isvisible');
                    });

                    if (ChatWindowFactory.isWindowMinimized() || ChatWindowFactory.isWindowPoppedOut()) {
                        if (window.opener) {
                            element.show();
                            window.focus();
                        }
                        else {
                            element.hide();
                        }
                    }

                    /**
                     * go to direct chat tab
                     * @param  {Event} event key presse event
                     * @return {boolean} true if handled
                     *
                     */
                    function goDirectToChatTab(event) {

                        var tabIndex = event.keyCode - 49, //gets the zero based index
                            openConversations = element.find('.origin-social-chatwindow-tab-overflow-menu-item,.origin-social-chatwindow-tab[data-conversation-id]'),
                            conversationId,
                            selectedConversationElement;

                        if (openConversations) {
                            selectedConversationElement = (tabIndex === 8) ? openConversations[openConversations.length-1]: openConversations[tabIndex];

                            if(selectedConversationElement) {
                                conversationId = selectedConversationElement.getAttribute('data-conversation-id');
                                ctrl.onRaiseConversation(conversationId);
                                return true;
                            }
                        }
                    }

                    /**
                     *  close the current chat tab
                     * @return {boolean} true if handled
                     *
                     */
                    function closeCurrentChatTab() {
                        if (Object.keys(scope.conversations).length === 1) {
                            if (ChatWindowFactory.isWindowPoppedOut() || scope.igoContext) {
                                window.close();
                            } else {
                                ChatWindowFactory.closeWindow();
                            }
                        } else {
                            ctrl.onCloseTabClick(element.find('.origin-social-chatwindow-tab.otktab-active'));
                        }
                        return true;
                    }

                    /**
                     *  minimize the chat window
                     * @return {boolean} true if handled
                     *
                     */
                    function minimizeWindowFromKeyboard() {
                        if(!ChatWindowFactory.isWindowPoppedOut() && !scope.igoContext) {
                            ChatWindowFactory.minimizeWindow();
                            return true;
                        }
                    }

                    /**
                     * listens for key presses to switch directly to a chat tab
                     * @return true if handled
                     */
                    function handleDirectChatShortcut(event) {
                        var modifierPressed = (Origin.utils.os() === 'MAC') ? event.metaKey && event.altKey : event.ctrlKey && event.altKey;
                        if (modifierPressed) {
                            return KeyEventsHelper.mapEvents(event, {
                                '1': goDirectToChatTab,
                                '2': goDirectToChatTab,
                                '3': goDirectToChatTab,
                                '4': goDirectToChatTab,
                                '5': goDirectToChatTab,
                                '6': goDirectToChatTab,
                                '7': goDirectToChatTab,
                                '8': goDirectToChatTab,
                                '9': goDirectToChatTab
                            });
                        }
                    }

                    /**
                     * listens for key presses to close tabs and the windows
                     * @return true if handled
                     */
                    function handleCloseWindowShortcut(event) {
                        var poppedOutModiferPressed = (Origin.utils.os() === 'MAC') ? event.metaKey && !event.ctrlKey : event.ctrlKey && !event.altKey,
                            dockedModifierPressed = (Origin.utils.os() === 'MAC') ? event.metaKey && event.ctrlKey : event.ctrlKey && event.altKey;
                        //if the window popped out the hotkey is just ctrl-w as opposed to ctrl-alt-w when it is not
                        if (ChatWindowFactory.isWindowPoppedOut() || scope.igoContext ? poppedOutModiferPressed: dockedModifierPressed) {
                            return KeyEventsHelper.mapEvents(event, {
                                'W': closeCurrentChatTab
                            });
                        }
                    }

                    /**
                     * listens for key presses to minimize the chat window
                     * @return true if handled
                     */
                    function handleMinimizeWindowShortcut(event) {
                        var modifierPressed = (Origin.utils.os() === 'MAC') ? event.metaKey && event.ctrlKey : event.ctrlKey && event.altKey;
                        if (modifierPressed) {
                            return KeyEventsHelper.mapEvents(event, {
                                'M': minimizeWindowFromKeyboard
                            });
                        }
                    }

                    /**
                     * all key down presses routed through here
                     * @param  {Event} event the keypress event logic
                     * @return {boolean} returns false if the ctrl & alt are pressed else undefined to allow it to bubble up
                     */
                    function handleKeyDown(event) {
                        //we handle these events in the key down to handle the command key on the mac which does not work with keyup

                        if(handleDirectChatShortcut(event)) {
                            //stop the propagation
                            return false;
                        }

                        if(handleCloseWindowShortcut(event)) {
                            //stop the propagation
                            return false;
                        }

                        if(handleMinimizeWindowShortcut(event)) {
                            //stop the propagation
                            return false;
                        }
                    }

                    /**
                     * clean up listeners
                     */
                    function onDestroy() {
                        Origin.events.off(Origin.events.CLIENT_SOCIAL_FOCUSONACTIVECHATWINDOW, onFocusOnActiveChatWindow);
                        element.off('keydown', handleKeyDown);
                    }

                    element.on('keydown', handleKeyDown);
                    scope.$on('$destroy', onDestroy);
                    window.addEventListener('unload', onDestroy);

                    Origin.events.on(Origin.events.CLIENT_SOCIAL_FOCUSONACTIVECHATWINDOW, onFocusOnActiveChatWindow);
                }
            };
        });
} ());
