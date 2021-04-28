(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageGamesCtrl($scope) {
        $scope.isSelf = ($scope.nucleusId === '') || (Number($scope.nucleusId) === Number(Origin.user.userPid()));
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageGames
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @description
     *
     * Profile Page - Games
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-games nucleusid="123456789"
     *         ></origin-profile-page-games>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageGamesCtrl', OriginProfilePageGamesCtrl)
        .directive('originProfilePageGames', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageGamesCtrl',
                scope: {
                    nucleusId: '@nucleusid'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/views/games.html')
            };

        });
}());

