(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageAchievementsCtrl($scope) {
        $scope.isSelf = ($scope.nucleusId === '') || (Number($scope.nucleusId) === Number(Origin.user.userPid()));
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageAchievements
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid - nucleusId of the user
     * @description
     *
     * Profile Page - Achievements
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-achievements nucleusid="123456789" achievementsetid="123456789"
     *         ></origin-profile-page-achievements>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageAchievementsCtrl', OriginProfilePageAchievementsCtrl)
        .directive('originProfilePageAchievements', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageAchievementsCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    achievementSetId: '@achievementsetid'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/views/achievements.html')
            };

        });
}());