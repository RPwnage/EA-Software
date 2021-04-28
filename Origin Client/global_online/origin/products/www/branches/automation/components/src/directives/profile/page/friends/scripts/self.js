(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageFriendsSelfCtrl($scope, RosterDataFactory, AuthFactory, UtilFactory, FindFriendsFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-friends-self';
        var BATCHSIZE = 50;

        $scope.noFriendsTitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsTitleStr, CONTEXT_NAMESPACE, 'nofriendstitlestr');
        $scope.noFriendsSubtitleLoc = UtilFactory.getLocalizedStr($scope.noFriendsSubtitleStr, CONTEXT_NAMESPACE, 'nofriendssubtitlestr');
        $scope.findFriendsLoc = UtilFactory.getLocalizedStr($scope.findFriendsStr, CONTEXT_NAMESPACE, 'findfriendsstr');
        $scope.showMoreLoc = UtilFactory.getLocalizedStr($scope.showMoreStr, CONTEXT_NAMESPACE, 'showmorestr');

        $scope.limit = 9;
        $scope.friendsLimit = $scope.limit;

        function onUpdate() {

            RosterDataFactory.getRoster('FRIENDS').then(function (roster) {

                $scope.allFriends = [];

                for (var id in roster) {
                    var friend = roster[id];
                    $scope.allFriends.push(friend);
                }

                var friendsTotal = $scope.allFriends.length;

                if (friendsTotal < $scope.friendsLimit) {
                    $scope.friendsLimit = friendsTotal;
                }

                $scope.myFriendsLoc = UtilFactory.getLocalizedStr($scope.myFriendsStr, CONTEXT_NAMESPACE, 'myfriendsstr')
                    .replace('%number%', $scope.friendsLimit)
                    .replace('%total%', friendsTotal);

                $scope.$digest();
            });
        }

        if (AuthFactory.isAppLoggedIn()) {
            onUpdate();
        }

        function onClientAuthChanged() {
            onUpdate();
        }

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onClientAuthChanged);
        AuthFactory.events.on('myauth:ready', onClientAuthChanged);

        function onDestroy() {
            AuthFactory.events.off('myauth:change', onClientAuthChanged);
            AuthFactory.events.off('myauth:ready', onClientAuthChanged);
        }
        $scope.$on('$destroy', onDestroy);

        $scope.findFriends = function () {
            // Show the Find Friends dialog
            FindFriendsFactory.findFriends();
        };

        $scope.showMore = function () {
            $scope.friendsLimit += BATCHSIZE;

            var friendsTotal = $scope.allFriends.length;
            if ($scope.friendsLimit >= friendsTotal) {
                $scope.friendsLimit = friendsTotal;
            }

            $scope.myFriendsLoc = UtilFactory.getLocalizedStr($scope.myFriendsStr, CONTEXT_NAMESPACE, 'myfriendsstr')
                .replace('%number%', $scope.friendsLimit)
                .replace('%total%', friendsTotal);

            $scope.$digest();
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageFriendsSelf
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} nofriendstitlestr "Gaming is better with friends"
     * @param {LocalizedString} nofriendssubtitlestr "Build your friends list to chat with friends, see what they're playing, join multiplayer games, and more."
     * @param {LocalizedString} findfriendsstr "Find Friends"
     * @param {LocalizedString} myfriendsstr "My Friends - %number% of %total%"
     * @param {LocalizedString} showmorestr "Show More"
     *
     * @description
     *
     * Profile Page - Friends - Self
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-friends-self
     *             nofriendstitlestr="Gaming is better with friends"
     *             nofriendssubtitlestr="Build your friends list to chat with friends, see what they're playing, join multiplayer games, and more."
     *             findfriendsstr="Find Friends"
     *             myfriendsstr="My Friends - %number% of %total%"
     *             showmorestr="Show More"
     *         ></origin-profile-page-friends-self>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageFriendsSelfCtrl', OriginProfilePageFriendsSelfCtrl)
        .directive('originProfilePageFriendsSelf', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageFriendsSelfCtrl',
                scope: {
                    noFriendsTitleStr: '@nofriendstitlestr',
                    noFriendsSubtitleStr: '@nofriendssubtitlestr',
                    findFriendsStr: '@findfriendsstr',
                    myFriendsStr: '@myfriendsstr',
                    showMoreStr: '@showmorestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/friends/views/self.html')
            };

        });
}());

