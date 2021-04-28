(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageFriendsCtrl($scope) {
        $scope.isSelf = ($scope.nucleusId === '') || (Number($scope.nucleusId) === Number(Origin.user.userPid()));
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageFriends
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @description
     *
     * Profile Page - Friends
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-friends nucleusid="123456789"
     *         ></origin-profile-page-friends>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageFriendsCtrl', OriginProfilePageFriendsCtrl)
        .directive('originProfilePageFriends', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageFriendsCtrl',
                scope: {
                    nucleusId: '@nucleusid'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/views/friends.html')
            };

        });
}());

