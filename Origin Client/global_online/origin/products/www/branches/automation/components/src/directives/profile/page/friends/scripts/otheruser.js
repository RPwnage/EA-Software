(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageFriendsOtheruserCtrl($scope, ComponentsLogFactory, RosterDataFactory, UtilFactory, AuthFactory, ObserverFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-friends-otheruser';
        var BATCHSIZE = 50;
        var SPINNER_THRESHOLD = 200; // If the user has more than this amount of friends we will show the friends as they load in, otherwise we will simply show a spinner until all friends have loaded
        var currentFriendPage = 0;
        var loadingData = false;

        $scope.privateLoc = UtilFactory.getLocalizedStr($scope.privateStr, CONTEXT_NAMESPACE, 'privatestr');
        $scope.showMoreLoc = UtilFactory.getLocalizedStr($scope.showMoreStr, CONTEXT_NAMESPACE, 'showmorestr');
        $scope.noFriendsSubtitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsSubtitleStr, CONTEXT_NAMESPACE, 'nofriendssubtitlestr');
        $scope.errorTitleLoc = UtilFactory.getLocalizedStr($scope.errorTitleStr, CONTEXT_NAMESPACE, 'errortitlestr');
        $scope.errorDescriptionLoc = UtilFactory.getLocalizedStr($scope.errorDescriptionStr, CONTEXT_NAMESPACE, 'errordescriptionstr');
        $scope.errorButtonLoc = UtilFactory.getLocalizedStr($scope.errorButtonStr, CONTEXT_NAMESPACE, 'errorbuttonstr');

        $scope.usersFriendsInCommon = [];
        $scope.usersFriendsOther = [];
        $scope.limit = 9;
        $scope.friendsInCommonLimit = $scope.limit;
        $scope.friendsOtherLimit = $scope.limit;
        $scope.privacy = { isPrivate: false };
        $scope.loadingState = true;
        $scope.errorState = false;

		RosterDataFactory.getRosterUser($scope.nucleusId).then(function(user) {
			if (user) {
				var observer = ObserverFactory.create(user.privacy)
					.defaultTo({isPrivate: false});
                $scope.noFriendsTitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsTitleStr,
                    CONTEXT_NAMESPACE,
                    'nofriendstitlestr', {'%username%' : user.originId});

				observer.attachToScope($scope, 'privacy');
				observer.attachToScope($scope, onUpdate);
			}
		});

		function processMyRoster(roster) {
		    $scope.roster = roster;

		    // Now get the user's total friend count
		    Origin.atom.atomFriendCountForUser($scope.nucleusId)
                .then(processUsersFriendCount)
                .catch(handleUsersFriendCountError);
        }


        function processUsersFriends(response) {

            var friendsBatch = response;
            if (friendsBatch.length > 0) {

                if ($scope.usersFriendCount >= SPINNER_THRESHOLD) {
                    // Start displaying results now
                    $scope.loadingState = false;
                }

                // Catagorize the new batch of friends, avoiding duplicates

                friendsBatch.map(function (newFriend) {
                    var exists = false, existingFriend, i;
                    if ($scope.roster[newFriend.userId] !== undefined) {
                        for (i = 0; i < $scope.usersFriendsInCommon.length; i++) {
                            existingFriend = $scope.usersFriendsInCommon[i];
                            if (existingFriend.userId === newFriend.userId) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            $scope.usersFriendsInCommon.push(newFriend);
                        }
                    }
                    else {
                        for (i = 0; i < $scope.usersFriendsOther.length; i++) {
                            existingFriend = $scope.usersFriendsOther[i];
                            if (existingFriend.userId === newFriend.userId) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            $scope.usersFriendsOther.push(newFriend);
                        }
                    }
                });

                var commonFriendsTotal = $scope.usersFriendsInCommon.length;
                var uncommonFriendsTotal = $scope.usersFriendsOther.length;

                if (commonFriendsTotal < $scope.friendsInCommonLimit) {
                    $scope.friendsInCommonLimit = commonFriendsTotal;
                }
                if ((commonFriendsTotal > $scope.friendsInCommonLimit) && ($scope.friendsInCommonLimit < $scope.limit)) {
                    $scope.friendsInCommonLimit = commonFriendsTotal;
                    if ($scope.friendsInCommonLimit > $scope.limit) {
                        $scope.friendsInCommonLimit = $scope.limit;
                    }
                }

                if (uncommonFriendsTotal < $scope.friendsOtherLimit) {
                    $scope.friendsOtherLimit = uncommonFriendsTotal;
                }
                if ((uncommonFriendsTotal > $scope.friendsOtherLimit) && ($scope.friendsOtherLimit < $scope.limit)) {
                    $scope.friendsOtherLimit = uncommonFriendsTotal;
                    if ($scope.friendsOtherLimit > $scope.limit) {
                        $scope.friendsOtherLimit = $scope.limit;
                    }
                }

                // Update labels

                $scope.friendsInCommonLoc = UtilFactory.getLocalizedStr($scope.friendsInCommonStr, CONTEXT_NAMESPACE, 'friendsincommonstr')
                    .replace('%number%', $scope.friendsInCommonLimit)
                    .replace('%total%', commonFriendsTotal);

                $scope.otherFriendsLoc = UtilFactory.getLocalizedStr($scope.otherFriendsStr, CONTEXT_NAMESPACE, 'otherfriendsstr')
                    .replace('%number%', $scope.friendsOtherLimit)
                    .replace('%total%', uncommonFriendsTotal);

                $scope.$digest();

                // Get the next page of friends

                ++currentFriendPage;
                Origin.atom.atomFriendsForUser($scope.nucleusId, currentFriendPage)
                    .then(processUsersFriends)
                    .catch(handleUsersFriendsError);
            }
            else {
                // ALL DONE
                loadingData = false;
                $scope.loadingState = false;
                $scope.$digest();
            }
        }

        function handleUsersFriendsError(error) {
            if (error.message.indexOf('403') >= 0) {
                RosterDataFactory.setProfilePrivacy($scope.nucleusId, true);
            }
            else {
                $scope.errorState = true;
            }
            loadingData = false;
            $scope.loadingState = false;
            $scope.$digest();
            ComponentsLogFactory.error('origin-profile-page-friends-otheruser atomFriendsForUser:', error);
        }

        function processUsersFriendCount(response) {

            RosterDataFactory.setProfilePrivacy($scope.nucleusId, false);

            $scope.usersFriendCount = response;

            // Get ready to get and start sorting the friends
            $scope.usersFriendsInCommon = [];
            $scope.usersFriendsOther = [];

            if ($scope.usersFriendCount > 0) {
                // Get the first page of the user's friends
                Origin.atom.atomFriendsForUser($scope.nucleusId, currentFriendPage)
                    .then(processUsersFriends)
                    .catch(handleUsersFriendsError);
            }
            else {
                // ALL DONE
                loadingData = false;
                $scope.loadingState = false;
                $scope.$digest();
            }
        }

        function handleUsersFriendCountError(error) {
            if (error.message.indexOf('403') >= 0) {
                RosterDataFactory.setProfilePrivacy($scope.nucleusId, true);
            }
            else {
                $scope.errorState = true;
            }
            loadingData = false;
            $scope.loadingState = false;
            $scope.$digest();
            ComponentsLogFactory.error('origin-profile-page-friends-otheruser atomFriendCountForUser:', error);
        }

        function onUpdate() {
            if (AuthFactory.isAppLoggedIn()) {
                if (!loadingData) {
                    loadingData = true;
                    currentFriendPage = 0;
                    $scope.loadingState = true;
                    $scope.errorState = false;
                    // First get our roster
                    RosterDataFactory.getRoster('FRIENDS').then(processMyRoster);
                }
            }
        }

        if (AuthFactory.isAppLoggedIn()) {
            onUpdate();
        }

        function onClientAuthChanged(authChangeObject) {
            authChangeObject = authChangeObject;
            onUpdate();
            $scope.$digest();
        }

		ObserverFactory.create(RosterDataFactory.getBlockListWatch())
			.attachToScope($scope, onUpdate);

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onClientAuthChanged);
        AuthFactory.events.on('myauth:ready', onClientAuthChanged);

        function onDestroy() {
            AuthFactory.events.off('myauth:change', onClientAuthChanged);
            AuthFactory.events.off('myauth:ready', onClientAuthChanged);
        }
        $scope.$on('$destroy', onDestroy);

        $scope.showMoreCommon = function () {
            $scope.friendsInCommonLimit += BATCHSIZE;

            var commonFriendsTotal = $scope.usersFriendsInCommon.length;
            if ($scope.friendsInCommonLimit >= commonFriendsTotal) {
                $scope.friendsInCommonLimit = commonFriendsTotal;
            }

            $scope.friendsInCommonLoc = UtilFactory.getLocalizedStr($scope.friendsInCommonStr, CONTEXT_NAMESPACE, 'friendsincommonstr')
                .replace('%number%', $scope.friendsInCommonLimit)
                .replace('%total%', commonFriendsTotal);

            $scope.$digest();
        };

        $scope.showMoreUncommon = function () {
            $scope.friendsOtherLimit += BATCHSIZE;

            var uncommonFriendsTotal = $scope.usersFriendsOther.length;
            if ($scope.friendsOtherLimit >= uncommonFriendsTotal) {
                $scope.friendsOtherLimit = uncommonFriendsTotal;
            }

            $scope.otherFriendsLoc = UtilFactory.getLocalizedStr($scope.otherFriendsStr, CONTEXT_NAMESPACE, 'otherfriendsstr')
                .replace('%number%', $scope.friendsOtherLimit)
                .replace('%total%', uncommonFriendsTotal);

            $scope.$digest();
        };

        $scope.onTryAgain = function () {
            if (!loadingData) {
                loadingData = true;
                $scope.errorState = false;
                $scope.loadingState = true;
                // First get our roster
                RosterDataFactory.getRoster('FRIENDS').then(processMyRoster);
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageFriendsOtheruser
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @param {LocalizedString} friendsincommonstr "Friends in Common - %number% of %total%"
     * @param {LocalizedString} otherfriendsstr "Other Friends - %number% of %total%"
     * @param {LocalizedString} showmorestr "Show More"
     * @param {LocalizedString} privatestr "You do not have permission to view this profile."
     * @param {LocalizedString} nofriendstitlestr "Gaming is better with friends"
     * @param {LocalizedString} nofriendssubtitlestr "Add this person to your friends list and invite them to a game."
     * @param {LocalizedString} errortitlestr "Well, that didn't go as planned"
     * @param {LocalizedString} errordescriptionstr "Origin encountered an issue loading this page. Please try again later."
     * @param {LocalizedString} errorbuttonstr "Try Again"
     * @description
     *
     * Profile Page - Friends - Other user
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-friends-otheruser nucleusid="123456789"
     *            friendsincommonstr="Friends in Common - %number% of %total%"
     *            otherfriendsstr="Other Friends - %number% of %total%"
     *            showmorestr="Show More"
     *            privatestr="You do not have permission to view this profile."
     *            nofriendstitlestr="Gaming is better with friends"
     *            nofriendssubtitlestr="Add this person to your friends list and invite them to a game."
     *            errortitlestr="Well, that didn't go as planned"
     *            errordescriptionstr="Origin encountered an issue loading this page. Please try again later."
     *            errorbuttonstr="Try Again"
     *         ></origin-profile-page-friends-otheruser>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageFriendsOtheruserCtrl', OriginProfilePageFriendsOtheruserCtrl)
        .directive('originProfilePageFriendsOtheruser', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageFriendsOtheruserCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    friendsInCommonStr: '@friendsincommonstr',
                    otherFriendsStr: '@otherfriendsstr',
                    showMoreStr: '@showmorestr',
                    privateStr: '@privatestr',
                    noFriendsTitleStr: '@nofriendstitlestr',
                    noFriendsSubtitleStr: '@nofriendssubtitlestr',
                    errorTitleStr: '@errortitlestr',
                    errorDescriptionStr: '@errordescriptionstr',
                    errorButtonStr: '@errorbuttonstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/friends/views/otheruser.html')
            };

        });
}());

