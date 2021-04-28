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
	function OriginSocialHubRosterAreaCtrl($scope, $timeout, RosterDataFactory, UtilFactory, AuthFactory, SocialHubFactory) {

	    var loadTimeoutTimer,
        handles = [];

	    var CONTEXT_NAMESPACE = 'origin-social-hub-roster-area';

		 var callThrottledScopeDigest = UtilFactory.throttle(function() {
			 $timeout(function() { $scope.$digest(); }, 0, false);
		 }, 2000);

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

		$scope.filterText = '';

        // for now assume that if we have extra parameters on the window it's an OIG window
        var windowParams = window.extraParams;
        if (windowParams) {
            SocialHubFactory.setOIGContext(true);
        }

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
            window.clearTimeout(loadTimeoutTimer);
			RosterDataFactory.getNumFriends().then(function(count) {
				var rosterState = $scope.rosterState;
				if (count > 0) {
				    $scope.rosterState = 'LOADED';
				} else {
					$scope.rosterState = 'NOFRIENDS';
				}

				if (rosterState !== $scope.rosterState) {
					callThrottledScopeDigest();
				}
			});

			calculateViewportFriendCount();

			var rosterLoadStartTime = RosterDataFactory.getRosterLoadStartTime();
			if( rosterLoadStartTime ) {
				//console.log('TR: rosterLoadPerformanceDirective:  milliseconds:' + (Date.now() - rosterLoadStartTime - 5000));
			}

        }

        function onRosterError() {
            window.clearTimeout(loadTimeoutTimer);
            $scope.rosterState = 'ERROR';
            $scope.$digest();
        }

		function onRosterUpdate() {
		    if (Origin.xmpp.isConnected()) {
		        window.clearTimeout(loadTimeoutTimer);
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

        function onRosterResults() {
            window.clearTimeout(loadTimeoutTimer);
            $scope.rosterState = 'LOADED';
            $scope.$digest();
        }

		function onClientOnlineStateChanged(onlineObj) {
			startLoadingState();
			$scope.$digest();

			if (onlineObj && onlineObj.isOnline) {
				RosterDataFactory.getRoster('ALL').then(onRosterLoaded, onRosterError);
			}
		}

		function onXmppConnect() {
            startLoadingState();
            $scope.$digest();
        }

        function onXmppDisconnect() {
            startLoadingState();
            $scope.$digest();
        }
        handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

		function onUnpopOutHub() {
            window.clearTimeout(loadTimeoutTimer);
        }

		RosterDataFactory.getRoster('ALL').then(onRosterLoaded);

        handles[handles.length] = RosterDataFactory.events.on('socialRosterLoaded', onRosterLoaded);
        handles[handles.length] = RosterDataFactory.events.on('socialRosterResults', onRosterResults);
        handles[handles.length] = RosterDataFactory.events.on('socialRosterLoadError', onRosterError);
		handles[handles.length] = RosterDataFactory.events.on('socialRosterUpdateALL', onRosterUpdate);
		handles[handles.length] = RosterDataFactory.events.on('socialRosterDeleteALL', onRosterUpdate);
		handles[handles.length] = RosterDataFactory.events.on('xmppConnected', onXmppConnect);
		handles[handles.length] = RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);
        handles[handles.length] = SocialHubFactory.events.on('unpopOutHub', onUnpopOutHub);

		this.getRoster = function() {
			RosterDataFactory.getRoster('ALL').then(onRosterLoaded, onRosterError);
		};

		function startLoadingState() {
		    $scope.rosterState = 'LOADING';

		    // Start a timeout
		    window.clearTimeout(loadTimeoutTimer);
		    loadTimeoutTimer = window.setTimeout(function () {
		        $scope.rosterState = 'TIMEOUT';
		        $scope.$digest();
		    }, 30000); // 30 seconds
		}

        // This is for child windows so they are aware of the rosters status
        if (RosterDataFactory.isRosterLoaded()) {
            onRosterLoaded();
        }

		startLoadingState();

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
     *         ></origin-social-hub-roster-area>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterAreaCtrl', OriginSocialHubRosterAreaCtrl)
        .directive('originSocialHubRosterArea', function (ComponentsConfigFactory, NavigationFactory) {

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
                    friendsStr: '@friendsstr'
                },
                controller: 'OriginSocialHubRosterAreaCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/area.html'),
                link: function(scope, element, attrs, ctrl) {

                    // Filter
                    $(element).on('keyup', '.origin-social-hub-roster-area-friends-filter-group-field > input', function() {
                        // PREET: You can probably do this by going $(e.target).val();
                        // or maybe even $(this).val()
                        var text = $(element).find('.origin-social-hub-roster-area-friends-filter-group-field > input').val();
                        ctrl.setFriendsListFilter(text);
                    });

					// handle the clear filter icon click
					$('.origin-social-hub-roster-area-friends-clear-filter').click(function() {
						ctrl.setFriendsListFilter('');
						$(element).find('.origin-social-hub-roster-area-friends-filter-group-input').val('');
					});

					$('.origin-social-hub-roster-area-viewport').scroll(ctrl.handleScroll);

					$(window).bind('resize', ctrl.handleWindowResize);

					ctrl.setViewportElement($('.origin-social-hub-roster-area-viewport'));

                    // Listen for clicks on the "find friends" button
                    $(element).on('click', '.origin-social-hub-roster-area-friends-add-friends', function () {
                        NavigationFactory.searchForFriends();
                    });

                    // Prevents page from scrolling while
                    // scrolling inside viewport area
                    $('.origin-social-hub-roster-area-viewport').bind('mousewheel', function (e) {

                        $(this).scrollTop($(this).scrollTop() - e.originalEvent.wheelDeltaY);

                        //prevent page fom scrolling
                        return false;
                    });

                    // Listen for clicks on the "error loading roster" cta button
                    $(element).on('buttonClicked', '.origin-social-hub-roster-area-list-error origin-social-hub-cta', function() {
                        window.alert('FEATURE NOT IMPLEMENTED - Error Loading Roster CTA Button Clicked');
                    });

                    // Listen for clicks on the "you have no friends" cta button
                    $(element).on('buttonClicked', '.origin-social-hub-roster-area-list-nofriends origin-social-hub-cta', function() {
                        NavigationFactory.searchForFriends();
                    });

					ctrl.getRoster();
                }

            };

        });
}());
