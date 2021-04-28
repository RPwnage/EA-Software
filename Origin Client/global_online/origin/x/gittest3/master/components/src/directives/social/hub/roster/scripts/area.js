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
	function OriginSocialHubRosterAreaCtrl($scope, RosterDataFactory, UtilFactory) {

	    var CONTEXT_NAMESPACE = 'origin-social-hub-roster-area';

        $scope.searchFriendsLoc = UtilFactory.getLocalizedStr($scope.searchFriendsStr, CONTEXT_NAMESPACE, 'searchfriendsstr');
        $scope.errorLoadingFriendsListCtaTitleLoc = UtilFactory.getLocalizedStr($scope.errorLoadingFriendsListCtaTitleStr, CONTEXT_NAMESPACE, 'errorloadingfriendslistctatitlestr');
        $scope.errorLoadingFriendsListCtaDescLoc = UtilFactory.getLocalizedStr($scope.errorLoadingFriendsListCtaDescStr, CONTEXT_NAMESPACE, 'errorloadingfriendslistctadescstr');
        $scope.noFriendsCtaTitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsCtaTitleStr, CONTEXT_NAMESPACE, 'nofriendsctatitlestr');
        $scope.noFriendsCtaDescLoc = UtilFactory.getLocalizedStr($scope.noFriendsCtaDescStr, CONTEXT_NAMESPACE, 'nofriendsctadescstr');
        $scope.noFriendsCtaButtonLoc = UtilFactory.getLocalizedStr($scope.noFriendsCtaButtonStr, CONTEXT_NAMESPACE, 'nofriendsctabuttonstr');
        $scope.friendRequestsReceivedLoc = UtilFactory.getLocalizedStr($scope.friendRequestsReceivedStr, CONTEXT_NAMESPACE, 'friendrequestsreceivedstr');
        $scope.pendingApprovalLoc = UtilFactory.getLocalizedStr($scope.pendingApprovalStr, CONTEXT_NAMESPACE, 'pendingapprovalstr');
        $scope.favoritesLoc = UtilFactory.getLocalizedStr($scope.favoritesStr, CONTEXT_NAMESPACE, 'favoritesstr');
        $scope.friendsLoc = UtilFactory.getLocalizedStr($scope.friendsStr, CONTEXT_NAMESPACE, 'friendsstr');

        $scope.rosterState = 'LOADING';
		$scope.filterText = '';

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
		
		function handleResizeEvent() {
			if ($viewportElement) {
				RosterDataFactory.setRosterViewportHeight($viewportElement.height());
				calculateViewportFriendCount();
			}
		}
		
		this.setViewportElement = function($element) {
			$viewportElement = $element;
			RosterDataFactory.setRosterViewportHeight($viewportElement.height());
		};
		
		RosterDataFactory.events.on('updateRosterViewportHeight', calculateViewportFriendCount);
		RosterDataFactory.events.on('updateFriendPanelHeight', calculateViewportFriendCount);
		RosterDataFactory.events.on('jssdkLogin', calculateViewportFriendCount);
		
		this.handleScroll = UtilFactory.reverseThrottle(onThrottledScrollEvent, 100);
		
		this.handleWindowResize = UtilFactory.reverseThrottle(handleResizeEvent);

        this.setFriendsListFilter = function(filterText) {
            RosterDataFactory.setFriendsListFilter(filterText);
        };
        
        function onRosterLoaded() {
            RosterDataFactory.getNumFriends().then(function (numFriends) {
                
                if (numFriends === 0) {
                    $scope.rosterState = 'NOFRIENDS';
                } else {
                    $scope.rosterState = 'LOADED';
                }
                $scope.$digest();
            });
			
			var rosterLoadStartTime = RosterDataFactory.getRosterLoadStartTime();
			if( rosterLoadStartTime ) {
				console.log('TR: rosterLoadPerformanceDirective:  milliseconds:' + (Date.now() - rosterLoadStartTime - 5000));
			}
			
        }

        function onRosterError() {
            $scope.rosterState = 'ERROR';
            $scope.$digest();
        }

		function onRosterUpdate() {
			console.log('TR onRosterUpdate: ');
			RosterDataFactory.getRoster('ALL').then(function(roster) {
				console.log('TR onRosterUpdate: ' + Object.keys(roster).length );
				if (Object.keys(roster).length > 0) {
					$scope.rosterState = 'LOADED';
				} else if (!Object.keys(roster).length) {
					$scope.rosterState = 'NOFRIENDS';
				}
				$scope.$digest();
			});
		}

        function onRosterResults() {
            $scope.rosterState = 'LOADED';
            $scope.$digest();
        }

        RosterDataFactory.getRoster('ALL').then(onRosterLoaded);
        RosterDataFactory.events.on('socialRosterResults', onRosterResults);
        RosterDataFactory.events.on('socialRosterLoadError', onRosterError);	
		RosterDataFactory.events.on('socialRosterUpdateALL', onRosterUpdate);
		RosterDataFactory.events.on('socialRosterDeleteALL', onRosterUpdate);

		this.getRoster = function() {
			RosterDataFactory.getRoster('ALL');
		};
		

        function handleUpdateFilter(filter) {
            $scope.filterText = filter;
			$scope.$digest();
        }
        
        RosterDataFactory.events.on('updateFriendsListFilter', handleUpdateFilter);		

        function onDestroy() {
            RosterDataFactory.events.off('updateRosterViewportHeight', calculateViewportFriendCount);
            RosterDataFactory.events.off('updateFriendPanelHeight', calculateViewportFriendCount);
            RosterDataFactory.events.off('jssdkLogin', calculateViewportFriendCount);
            RosterDataFactory.events.off('socialRosterResults', onRosterResults);
            RosterDataFactory.events.off('socialRosterLoadError', onRosterError);
            RosterDataFactory.events.off('socialRosterUpdateALL', onRosterUpdate);
            RosterDataFactory.events.off('socialRosterDeleteALL', onRosterUpdate);
            RosterDataFactory.events.off('updateFriendsListFilter', handleUpdateFilter);
        }

        $scope.$on('$destroy', onDestroy);
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
        .directive('originSocialHubRosterArea', function(ComponentsConfigFactory) {
        
            return {
                restrict: 'E',
                scope: {
                    searchFriendsStr: '@searchfriendsstr',
                    errorLoadingFriendsListCtaTitleStr: '@errorloadingfriendslistctatitlestr',
                    errorLoadingFriendsListCtaDescStr: '@errorloadingfriendslistctadescstr',
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
                    $(element).on('keyup', '.origin-social-hub-roster-area-friends-filter-group-input', function() {                         
                        var text = $(element).find('.origin-social-hub-roster-area-friends-filter-group-input').val();
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

                    // Prevents document from scrolling while inside chat area
                    $(document).on('keydown','.origin-social-hub-roster-area-viewport', function (e) {
                            e.preventDefault();
                        });

                    // Listen for clicks on the "error loading roster" cta button
                    $(element).on('buttonClicked', '.origin-social-hub-roster-area-list-error origin-social-hub-cta', function() { 
                        window.alert('FEATURE NOT IMPLEMENTED - Error Loading Roster CTA Button Clicked');
                    });

                    // Listen for clicks on the "you have no friends" cta button
                    $(element).on('buttonClicked', '.origin-social-hub-roster-area-list-nofriends origin-social-hub-cta', function() { 
                        window.alert('FEATURE NOT IMPLEMENTED - No Friends CTA Button Clicked');
                    });

					ctrl.getRoster();
                }
                
            };
        
        });
}());

