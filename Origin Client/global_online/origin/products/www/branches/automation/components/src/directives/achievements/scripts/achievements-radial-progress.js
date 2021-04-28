(function () {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    /**
     * Achievements Radial Progress Bar Ctrl
     */
    function OriginAchievementsRadialProgressCtrl($scope) {

        /**
         * Update Rotation - Radial Progress
         */
        $scope.degrees = $scope.totalPossible === 0? 0: Math.round(360.0 * $scope.score / $scope.totalPossible);

        // Handle out-of-bounds values for degrees
        if ($scope.degrees > 360) {
            $scope.degrees = 360;
        } else if ($scope.degrees < 0 || !$scope.degrees) {
            $scope.degrees = 0;
        }

        $scope.rotationDegreesCSS = {
            'transform': 'rotate(' + $scope.degrees + 'deg)',
            '-webkit-transform': 'rotate(' + $scope.degrees + 'deg)',
            '-ms-transform' : 'rotate(' + $scope.degrees + 'deg)'
        };
    }

    /**
     * Achievements Radial Progress Bar Directive
     */
    function originAchievementsRadialProgress(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                type: '@',
                totalPossible: '@totalpossible',
                score: '@score',
                progressSize: '@progresssize',
                textSize: '@textsize',
                text: '@',
                showratio: '=',
                theme: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('achievements/views/achievements-radial-progress.html'),
            controller: 'OriginAchievementsRadialProgressCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAchievementsRadialProgress
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} type - achievements or points
     * @param {string} progresssize - size of the radial progress : small, medium, or large.
     * @param {string} textsize - size of the text for achievements and points completed : small, medium, or large.
     * @param {string} textcolor - override the color of the text
     * @param {string} totalpossible - Total achievements or total points
     * @param {string} score - gained achievements or total points completed
     * @param {LocalizedString} text "Achievements completed" or "Origin points earned"
     * @param {BooleanEnumeration} showratio should you display the score as a ratio
     *
     * @description
     *
     * Radial Progress Bar Used For Achievements
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-achievements-radial-progress
                    size="large" type="achievements"  showratio="true" text="Achievements Completed">
                </origin-achievements-radial-progress>
     *     </file>
     * </example>
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginAchievementsRadialProgressCtrl', OriginAchievementsRadialProgressCtrl)
        .directive('originAchievementsRadialProgress', originAchievementsRadialProgress);
}());