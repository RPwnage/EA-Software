/**
 * Created by tirhodes on 2015-02-20.
 * @file area.js
 */
(function () {

    'use strict';
	var $viewportElement = null;

    /**
    * The controller
    */
	function OriginSocialHubRosterAreaCtrl($scope, $timeout, RosterDataFactory, UtilFactory, AuthFactory, SocialHubFactory, ObserverFactory, FindFriendsFactory) {

        var handles = [];

        var callThrottledScopeDigest = UtilFactory.throttle(function() {
	        $timeout(function() { $scope.$digest(); }, 0, false);
        }, 2000);

        var CONTEXT_NAMESPACE = 'origin-social-hub-roster-area';

        //Max #of Characters an account can be
        $scope.maxFilterLen = 16;

        //Data for calculating when to display no friends in current filter
        $scope.rosterCountDirty = true;
        $scope.totalRosterLength = 0;
        $scope.childSectionsRosterTotals = [];

        $scope.filterText = '';

        $scope.searchFriendsLoc = UtilFactory.getLocalizedStr($scope.searchFriendsStr, CONTEXT_NAMESPACE, 'searchfriendsstr');
        $scope.errorLoadingFriendsListCtaTitleLoc = UtilFactory.getLocalizedStr($scope.errorLoadingFriendsListCtaTitleStr, CONTEXT_NAMESPACE, 'errorloadingfriendslistctatitlestr');
        $scope.errorLoadingFriendsListCtaDescLoc = UtilFactory.getLocalizedStr($scope.errorLoadingFriendsListCtaDescStr, CONTEXT_NAMESPACE, 'errorloadingfriendslistctadescstr');
        $scope.timeoutLoc = UtilFactory.getLocalizedStr($scope.timeoutStr, CONTEXT_NAMESPACE, 'timeoutstr');
        $scope.noFriendsCtaTitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsCtaTitleStr, CONTEXT_NAMESPACE, 'nofriendsctatitlestr');
        $scope.noFriendsCtaDescLoc = UtilFactory.getLocalizedStr($scope.noFriendsCtaDescStr, CONTEXT_NAMESPACE, 'nofriendsctadescstr');
        $scope.noFriendsCtaButtonLoc = UtilFactory.getLocalizedStr($scope.noFriendsCtaButtonStr, CONTEXT_NAMESPACE, 'nofriendsctabuttonstr');
        $scope.friendRequestsReceivedLoc = UtilFactory.getLocalizedStr($scope.friendRequestsReceivedStr, CONTEXT_NAMESPACE, 'friendrequestsreceivedstr');
        $scope.pendingApprovalLoc = UtilFactory.getLocalizedStr($scope.pendingApprovalStr, CONTEXT_NAMESPACE, 'pendingapprovalstr');
        $scope.favoritesLoc = UtilFactory.getLocalizedStr($scope.favoritesStr, CONTEXT_NAMESPACE, 'favoritesstr');
        $scope.friendsLoc = UtilFactory.getLocalizedStr($scope.friendsStr, CONTEXT_NAMESPACE, 'friendsstr');
        $scope.noFilteredFriendTitleLoc = UtilFactory.getLocalizedStr($scope.noFilteredFriendTitleStr, CONTEXT_NAMESPACE, 'nofilteredfriendtitlestr', { '%filtertext%': $scope.filterText });
        $scope.noFilteredFriendBodyLoc = UtilFactory.getLocalizedStr($scope.noFilteredFriendBodyStr, CONTEXT_NAMESPACE, 'nofilteredfriendbodystr', { '%filtertext%': $scope.filterText });
        $scope.noFilteredFriendsCtaButtonLoc = UtilFactory.getLocalizedStr($scope.noFriendsCtaButtonStr, CONTEXT_NAMESPACE, 'nofilteredfriendsctabuttonstr');

        // listen for xmpp connect/disconnect
        ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
            .defaultTo({isConnected: Origin.xmpp.isConnected()})
            .attachToScope($scope, 'xmppConnection');


		function calculateViewportFriendCount() {
			var viewportHeight = RosterDataFactory.getRosterViewportHeight();
			if (!viewportHeight && $viewportElement) {
				viewportHeight = $viewportElement.height();
				RosterDataFactory.setRosterViewportHeight(viewportHeight);
			}


			var friendHeight = RosterDataFactory.getFriendPanelHeight();
			if (friendHeight > 0 && viewportHeight > 0) {
				RosterDataFactory.setSectionViewportFriendCount(Math.floor( (viewportHeight/friendHeight) ));
			}
		}

		function onThrottledScrollEvent() {
			RosterDataFactory.updateRosterViewport();
		}

		function onResizeEvent() {
			if ($viewportElement) {
				RosterDataFactory.setRosterViewportHeight($viewportElement.height());
				calculateViewportFriendCount();
			}
		}

		this.setViewportElement = function($element) {
			$viewportElement = $element;
			RosterDataFactory.setRosterViewportHeight($viewportElement.height());
		};

		handles[handles.length] = RosterDataFactory.events.on('updateRosterViewportHeight', calculateViewportFriendCount);
		handles[handles.length] = RosterDataFactory.events.on('updateFriendPanelHeight', calculateViewportFriendCount);
		handles[handles.length] = RosterDataFactory.events.on('jssdkLogin', calculateViewportFriendCount);

		this.handleScroll = UtilFactory.reverseThrottle(onThrottledScrollEvent, 100);

		this.handleWindowResize = UtilFactory.reverseThrottle(onResizeEvent, 100);

        this.setFriendsListFilter = function(filterText) {
            RosterDataFactory.setFriendsListFilter(filterText);
        };

        function onRosterLoaded() {

			calculateViewportFriendCount();
        }

        function onRosterError() {
        }

		function onRosterUpdate() {
		    if (Origin.xmpp.isConnected()) {
		        RosterDataFactory.getNumFriends().then(function (count) {
		            var rosterState = $scope.rosterState;
		            if (count > 0) {
		                $scope.rosterState = 'LOADED';
		            } else {
		                $scope.rosterState = 'NOFRIENDS';
		            }

		            if (rosterState !== $scope.rosterState) {
		                callThrottledScopeDigest();
		            }

		            calculateViewportFriendCount();
		        });
		    }
		}

		function onClientOnlineStateChanged(onlineObj) {
			$scope.$digest();

			if (onlineObj && onlineObj.isOnline) {
				RosterDataFactory.getRoster('ALL').then(onRosterLoaded, onRosterError);
			}
		}

        //Calculate the total friends visible across all child sections
        $scope.calculateTotalFilteredFriendCount = function(SectionID, FilteredTotal) {
            $scope.totalRosterLength -= $scope.childSectionsRosterTotals[SectionID];
            $scope.totalRosterLength += FilteredTotal;
            $scope.childSectionsRosterTotals[SectionID] = FilteredTotal;
            $scope.rosterCountDirty = false;
            if($scope.totalRosterLength === 0 && $scope.filterText.length > 0) {
                $scope.noFilteredFriendTitleLoc = UtilFactory.getLocalizedStr($scope.noFilteredFriendTitleStr, CONTEXT_NAMESPACE, 'nofilteredfriendtitlestr', { '%filtertext%': $scope.filterText });
                $scope.noFilteredFriendBodyLoc = UtilFactory.getLocalizedStr($scope.noFilteredFriendBodyStr, CONTEXT_NAMESPACE, 'nofilteredfriendbodystr', { '%filtertext%': $scope.filterText });
            }
            $scope.$digest();
        };
        //New Child Section created which needs added to our list
        $scope.getNewSectionID = function(FilteredTotal) {
            $scope.childSectionsRosterTotals.push(FilteredTotal);
            $scope.totalRosterLength += FilteredTotal;
            $scope.rosterCountDirty = false;
            if($scope.totalRosterLength === 0 && $scope.filterText.length > 0) {
                $scope.noFilteredFriendTitleLoc = UtilFactory.getLocalizedStr($scope.noFilteredFriendTitleStr, CONTEXT_NAMESPACE, 'nofilteredfriendtitlestr', { '%filtertext%': $scope.filterText });
                $scope.noFilteredFriendBodyLoc = UtilFactory.getLocalizedStr($scope.noFilteredFriendBodyStr, CONTEXT_NAMESPACE, 'nofilteredfriendbodystr', { '%filtertext%': $scope.filterText });
            }
            $scope.$digest();
            return ($scope.childSectionsRosterTotals.length - 1);
        };

        handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);


		RosterDataFactory.getRoster('ALL').then(onRosterLoaded, onRosterError);

        var rosterStatusObserver = ObserverFactory.create(RosterDataFactory.getRosterStatusWatch());
        rosterStatusObserver.attachToScope($scope, 'rosterLoadStatus');

		handles[handles.length] = RosterDataFactory.events.on('socialRosterUpdateALL', onRosterUpdate);
		handles[handles.length] = RosterDataFactory.events.on('socialRosterDeleteALL', onRosterUpdate);

		this.getRoster = function() {
			RosterDataFactory.getRoster('ALL').then(onRosterLoaded, onRosterError);
		};

		this.searchForFriends = function () {
		    // Show the Find Friends dialog
		    FindFriendsFactory.findFriends();
		};

        // This is for child windows so they are aware of the rosters status
        if (RosterDataFactory.isRosterLoaded()) {
            onRosterLoaded();
        }

        function handleUpdateFilter(filter) {
            $scope.filterText = filter;
            $scope.$digest();
        }

        handles[handles.length] = RosterDataFactory.events.on('updateFriendsListFilter', handleUpdateFilter);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubRosterArea
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} searchfriendsstr "Search Friends"
     * @param {LocalizedString} errorloadingfriendslistctatitlestr "Error"
     * @param {LocalizedString} errorloadingfriendslistctadescstr "Error loading the friends roster"
     * @param {LocalizedString} timeoutstr "Oops, we can't connect you to your friends list or chat right now. Hang tight, we'll get you chatting with friends as soon as we can."
     * @param {LocalizedString} nofriendsctatitlestr "Gaming is better with friends"
     * @param {LocalizedString} nofriendsctadescstr "Build your friends list to chat with friends, see what they're playing, join multiplayer games, and more."
     * @param {LocalizedString} nofriendsctabuttonstr "Find Friends"
     * @param {LocalizedString} friendrequestsreceivedstr "Friend Requests Received"
     * @param {LocalizedString} pendingapprovalstr "Pending Approval"
     * @param {LocalizedString} favoritesstr "Favorites"
     * @param {LocalizedString} friendsstr "Friends"
     * @param {LocalizedString} nofilteredfriendtitlestr "%filtertext% is not on your friends list"
     * @param {LocalizedString} nofilteredfriendbodystr "You can search for %filtertext% on Origin and add them as a friend."
     * @param {LocalizedString} nofilteredfriendsctabuttonstr "Search for a Friends"
     *
     *
     * @description
     *
     * origin social hub area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-roster-area
     *            searchfriendsstr="Search Friends"
     *            errorloadingfriendslistctatitlestr="Error"
     *            errorloadingfriendslistctadescstr="Error loading the friends roster"
     *            timeoutstr="Oops, we can't connect you to your friends list or chat right now. Hang tight, we'll get you chatting with friends as soon as we can."
     *            nofriendsctatitlestr="Gaming is better with friends"
     *            nofriendsctadescstr="Build your friends list to chat with friends, see what they're playing, join multiplayer games, and more."
     *            nofriendsctabuttonstr="Find Friends"
     *            friendrequestsreceivedstr="Friend Requests Received"
     *            pendingapprovalstr="Pending Approval"
     *            favoritesstr="Favorites"
     *            friendsstr="Friends"
     *            nofilteredfriendtitlestr="%filtertext% is not on your friends list"
     *            nofilteredfriendbodystr="You can search for %filtertext% on Origin and add them as a friend."
     *            nofilteredfriendsctabuttonstr="Search for a Friends"
     *         ></origin-social-hub-roster-area>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterAreaCtrl', OriginSocialHubRosterAreaCtrl)
        .directive('originSocialHubRosterArea', function (ComponentsConfigFactory, AnimateFactory) {

            return {
                restrict: 'E',
                scope: {
                    searchFriendsStr: '@searchfriendsstr',
                    errorLoadingFriendsListCtaTitleStr: '@errorloadingfriendslistctatitlestr',
                    errorLoadingFriendsListCtaDescStr: '@errorloadingfriendslistctadescstr',
                    timeoutStr: '@timeoutstr',
                    noFriendsCtaTitleStr: '@nofriendsctatitlestr',
                    noFriendsCtaDescStr: '@nofriendsctadescstr',
                    noFriendsCtaButtonStr: '@nofriendsctabuttonstr',
                    friendRequestsReceivedStr: '@friendrequestsreceivedstr',
                    pendingApprovalStr: '@pendingapprovalstr',
                    favoritesStr: '@favoritesstr',
                    friendsStr: '@friendsstr',
                    noFilteredFriendTitleStr: '@nofilteredfriendtitlestr',
                    noFilteredFriendBodyStr: '@nofilteredfriendbodystr',
                    noFilteredFriendsCtaButtonStr: '@nofilteredfriendsctabuttonstr'
                },
                controller: 'OriginSocialHubRosterAreaCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/area.html'),
                link: function(scope, element, attrs, ctrl) {

                    function onFocusOnFriendsList() {
                        element.find('.origin-social-hub-roster-area-friends-filter-group-field > input').focus();
                    }

                    // Filter
                    angular.element(element).on('keyup', '.origin-social-hub-roster-area-friends-filter-group-field > input', function() {
                        // PREET: You can probably do this by going angular.element(e.target).val();
                        // or maybe even angular.element(this).val()
                        var text = angular.element(element).find('.origin-social-hub-roster-area-friends-filter-group-field > input').val();
                        ctrl.setFriendsListFilter(text);
                    });

					// handle the clear filter icon click
					angular.element('.origin-social-hub-roster-area-friends-clear-filter').click(function() {
					    ctrl.setFriendsListFilter('');
						angular.element(element).find('.origin-social-hub-roster-area-friends-filter-group-field > input').val('');
					});

					angular.element('.origin-social-hub-roster-area-viewport').scroll(ctrl.handleScroll);

                    AnimateFactory.addResizeEventHandler(scope, ctrl.handleWindowResize, 800);

					ctrl.setViewportElement(angular.element('.origin-social-hub-roster-area-viewport'));

                    // Listen for clicks on the "find friends" button
                    angular.element(element).on('click', '.origin-social-hub-roster-area-friends-add-friends', function () {
                        ctrl.searchForFriends();
                    });

                    // Prevents page from scrolling while
                    // scrolling inside viewport area
                    angular.element('.origin-social-hub-roster-area-viewport').bind('mousewheel', function (e) {

                        angular.element(this).scrollTop(angular.element(this).scrollTop() - e.originalEvent.wheelDeltaY);

                        //prevent page fom scrolling
                        return false;
                    });

                    // Listen for clicks on the "error loading roster" cta button
                    angular.element(element).on('buttonClicked', '.origin-social-hub-roster-area-list-error origin-social-hub-cta', function() {
                        window.alert('FEATURE NOT IMPLEMENTED - Error Loading Roster CTA Button Clicked');
                    });

                    // Listen for clicks on the "you have no friends" cta button
                    angular.element(element).on('buttonClicked', '.origin-social-hub-roster-area-list-nofriends origin-social-hub-cta', function() {
                        ctrl.searchForFriends();
                    });

                    ctrl.getRoster();

                   function onDestroy() {
                        Origin.events.off(Origin.events.CLIENT_SOCIAL_FOCUSONFRIENDSLIST, onFocusOnFriendsList);
                    }

                    scope.$on('$destroy', onDestroy);
                    window.addEventListener('unload', onDestroy);

                    Origin.events.on(Origin.events.CLIENT_SOCIAL_FOCUSONFRIENDSLIST, onFocusOnFriendsList);
                }

            };

        });
}());
