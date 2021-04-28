(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageFriendsFriendtileCtrl($scope, $timeout, NavigationFactory, RosterDataFactory) {

        $scope.gamename = '';

        this.viewProfile = function () {
            NavigationFactory.goUserProfile($scope.nucleusId);
        };

        function handlePresenceChange(presence) {
            $scope.gamename = '';
            if (presence.show === 'ONLINE') {
                if (presence.gameActivity && presence.gameActivity.title !== undefined && presence.gameActivity.title !== '') {
                    // If we have a gameActivity object and a title we must be playing a game.
                    $scope.gamename = presence.gameActivity.title;
                }
            }
            $timeout(function () { $scope.$digest(); }, 0, false);
        }

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
        }
        $scope.$on('$destroy', onDestroy);

        RosterDataFactory.events.on('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);

        if (Number($scope.nucleusId) === Number(Origin.user.userPid())) {
            // This is me
            var presence = RosterDataFactory.getCurrentUserPresence();
            handlePresenceChange(presence);
        }
        else {
            RosterDataFactory.getFriendInfo($scope.nucleusId).then(function (user) {
                if (!!user && !!user.presence) {
                    handlePresenceChange(user.presence);
                }
            });
        }
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageFriendsFriendtile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @param {string} username username of the user
     * @description
     *
     * Profile Page - Friends - Friend Tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-friends-friendtile nucleusid="123456789"
     *             username="{{username}}"
     *         ></origin-profile-page-friends-friendtile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageFriendsFriendtileCtrl', OriginProfilePageFriendsFriendtileCtrl)
        .directive('originProfilePageFriendsFriendtile', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageFriendsFriendtileCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    username: '@username'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/friends/views/friendtile.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    // Listen for clicks on the "view profile" links
                    $(element).on('click', '.origin-profile-page-friends-friendtile-username', function () {
                        ctrl.viewProfile();
                    });

                }
            };

        });
}());

