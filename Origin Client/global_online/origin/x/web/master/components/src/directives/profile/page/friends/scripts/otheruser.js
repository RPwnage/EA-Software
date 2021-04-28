(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageFriendsOtheruserCtrl($scope, ComponentsLogFactory, RosterDataFactory, UtilFactory, AuthFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-friends-otheruser';

        $scope.privateLoc = UtilFactory.getLocalizedStr($scope.privateStr, CONTEXT_NAMESPACE, 'privatestr');
        $scope.showMoreLoc = UtilFactory.getLocalizedStr($scope.showMoreStr, CONTEXT_NAMESPACE, 'showmorestr');
        $scope.showLessLoc = UtilFactory.getLocalizedStr($scope.showLessStr, CONTEXT_NAMESPACE, 'showlessstr');
        $scope.noFriendsTitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsTitleStr, CONTEXT_NAMESPACE, 'nofriendstitlestr');
        $scope.noFriendsSubtitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsSubtitleStr, CONTEXT_NAMESPACE, 'nofriendssubtitlestr');

        $scope.usersFriends = [];
        $scope.usersFriendsInCommon = [];
        $scope.usersFriendsOther = [];
        $scope.limit = 9;
        $scope.friendsInCommonLimit = $scope.limit;
        $scope.friendsOtherLimit = $scope.limit;
        $scope.isPrivate = false;
        $scope.loading = false;

        function processMyRoster(roster) {

            $scope.usersFriendsInCommon = [];
            $scope.usersFriendsOther = [];

            $scope.usersFriends.map(function (friend) {
                if (roster[friend.userId] !== undefined) {
                    $scope.usersFriendsInCommon.push(friend);
                }
                else {
                    $scope.usersFriendsOther.push(friend);
                }
            });

            var commonFriendsTotal = $scope.usersFriendsInCommon.length;
            var uncommonFriendsTotal = $scope.usersFriendsOther.length;

            if (commonFriendsTotal < $scope.friendsInCommonLimit) {
                $scope.friendsInCommonLimit = commonFriendsTotal;
            }
            if (uncommonFriendsTotal < $scope.friendsOtherLimit) {
                $scope.friendsOtherLimit = uncommonFriendsTotal;
            }

            $scope.friendsInCommonLoc = UtilFactory.getLocalizedStr($scope.friendsInCommonStr, CONTEXT_NAMESPACE, 'friendsincommonstr')
                .replace('%number%', $scope.friendsInCommonLimit)
                .replace('%total%', commonFriendsTotal);

            $scope.otherFriendsLoc = UtilFactory.getLocalizedStr($scope.otherFriendsStr, CONTEXT_NAMESPACE, 'otherfriendsstr')
                .replace('%number%', $scope.friendsOtherLimit)
                .replace('%total%', uncommonFriendsTotal);
                
            $scope.loading = false;        
            $scope.isPrivate = false;            
            $scope.$digest();
        }

        function processUsersFriends(response) {
            RosterDataFactory.setProfilePrivacy(false); 
            $scope.usersFriends = response;

            // Now get my friends
            RosterDataFactory.getRoster('FRIENDS').then(processMyRoster);
        }

        function handleUsersFriendsError(error) {
            $scope.loading = false;                
            if (error.message.indexOf('403') >= 0) {
                $scope.isPrivate = true;
                RosterDataFactory.setProfilePrivacy(true);
            }
            $scope.$digest();
            ComponentsLogFactory.error('origin-profile-page-friends-otheruser atomFriendsForUser:', error.message);
        }

        function onUpdate() {

            $scope.usersFriends = [];

            // Get the user's friends
            Origin.atom.atomFriendsForUser($scope.nucleusId)
                .then(processUsersFriends)
                .catch(handleUsersFriendsError);
        }
        
        this.update = function() {
            $scope.loading = true;

            // Get the user's friends
            Origin.atom.atomFriendsForUser($scope.nucleusId)
                .then(processUsersFriends)
                .catch(handleUsersFriendsError);
        };

        if (AuthFactory.isAppLoggedIn()) {
            onUpdate();
        }

        function onClientAuthChanged(authChangeObject) {
            authChangeObject = authChangeObject;
            onUpdate();
            $scope.$digest();
        }

        function handleBlocklistChange() {
            onUpdate();
        }
        

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onClientAuthChanged);
        AuthFactory.events.on('myauth:ready', onClientAuthChanged);

        function onDestroy() {
            AuthFactory.events.off('myauth:change', onClientAuthChanged);
            AuthFactory.events.off('myauth:ready', onClientAuthChanged);
            RosterDataFactory.events.off('xmppBlockListChanged', handleBlocklistChange);
        }
        $scope.$on('$destroy', onDestroy);

        RosterDataFactory.events.on('xmppBlockListChanged', handleBlocklistChange);
        
        this.showMoreCommon = function () {
            var commonFriendsTotal = $scope.usersFriendsInCommon.length;
            $scope.friendsInCommonLimit = commonFriendsTotal;
            $scope.friendsInCommonLoc = UtilFactory.getLocalizedStr($scope.friendsInCommonStr, CONTEXT_NAMESPACE, 'friendsincommonstr')
                .replace('%number%', $scope.friendsInCommonLimit)
                .replace('%total%', commonFriendsTotal);
            $scope.$digest();
        };

        this.showMoreUncommon = function () {
            var uncommonFriendsTotal = $scope.usersFriendsOther.length;
            $scope.friendsOtherLimit = uncommonFriendsTotal;
            $scope.otherFriendsLoc = UtilFactory.getLocalizedStr($scope.otherFriendsStr, CONTEXT_NAMESPACE, 'otherfriendsstr')
                .replace('%number%', $scope.friendsOtherLimit)
                .replace('%total%', uncommonFriendsTotal);
            $scope.$digest();
        };

        this.showLessCommon = function () {
            var commonFriendsTotal = $scope.usersFriendsInCommon.length;
            $scope.friendsInCommonLimit = $scope.limit;
            if (commonFriendsTotal < $scope.friendsInCommonLimit) {
                $scope.friendsInCommonLimit = commonFriendsTotal;
            }
            $scope.friendsInCommonLoc = UtilFactory.getLocalizedStr($scope.friendsInCommonStr, CONTEXT_NAMESPACE, 'friendsincommonstr')
                .replace('%number%', $scope.friendsInCommonLimit)
                .replace('%total%', commonFriendsTotal);
            $scope.$digest();
        };

        this.showLessUncommon = function () {
            var uncommonFriendsTotal = $scope.usersFriendsOther.length;
            $scope.friendsOtherLimit = $scope.limit;
            if (uncommonFriendsTotal < $scope.friendsOtherLimit) {
                $scope.friendsOtherLimit = uncommonFriendsTotal;
            }
            $scope.otherFriendsLoc = UtilFactory.getLocalizedStr($scope.otherFriendsStr, CONTEXT_NAMESPACE, 'otherfriendsstr')
                .replace('%number%', $scope.friendsOtherLimit)
                .replace('%total%', uncommonFriendsTotal);
            $scope.$digest();
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageFriendsOtherUser
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @param {LocalizedString} friendsincommonstr "Friends in Common - %number% of %total%"
     * @param {LocalizedString} otherfriendsstr "Other Friends - %number% of %total%"
     * @param {LocalizedString} showmorestr "Show More"
     * @param {LocalizedString} showlessstr "Show Less"
     * @param {LocalizedString} privatestr "You do not have permission to view this profile."
     * @param {LocalizedString} nofriendstitlestr "Gaming is better with friends"
     * @param {LocalizedString} nofriendssubtitlestr "Add this person to your friends list and invite them to a game."
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
     *            showlessstr="Show Less"
     *            privatestr="You do not have permission to view this profile."
     *            nofriendstitlestr="Gaming is better with friends"
     *            nofriendssubtitlestr="Add this person to your friends list and invite them to a game."
     *         ></origin-profile-page-friends-otheruser>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageFriendsOtheruserCtrl', OriginProfilePageFriendsOtheruserCtrl)
        .directive('originProfilePageFriendsOtheruser', function (ComponentsConfigFactory, RosterDataFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageFriendsOtheruserCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    friendsInCommonStr: '@friendsincommonstr',
                    otherFriendsStr: '@otherfriendsstr',
                    showMoreStr: '@showmorestr',
                    showLessStr: '@showlessstr',
                    privateStr: '@privatestr',
                    noFriendsTitleStr: '@nofriendstitlestr',
                    noFriendsSubtitleStr: '@nofriendssubtitlestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/friends/views/otheruser.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;
                    
                    scope.$on('activePageChange', function(event, data) {
                        if (data === 'friends') {
                            // reload this page if this is a stranger profile. This is required in case this user has blocked me.
                            var rosterFriend = RosterDataFactory.getRosterFriend(scope.nucleusId);
                            var strangerProfile = (!rosterFriend || rosterFriend.subState !== 'BOTH');
                            if (strangerProfile) {
                                ctrl.update();
                            }
                        }
                    });

                    // Listen for clicks on the "show more" links
                    $(element).on('click', '.show-more-common', function () {
                        ctrl.showMoreCommon();
                    });
                    $(element).on('click', '.show-more-uncommon', function () {
                        ctrl.showMoreUncommon();
                    });

                    // Listen for clicks on the "show less" links
                    $(element).on('click', '.show-less-common', function () {
                        ctrl.showLessCommon();
                    });
                    $(element).on('click', '.show-less-uncommon', function () {
                        ctrl.showLessUncommon();
                    });

                }
            };

        });
}());

