(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-achievement-badge';

    /**
     * Achievement Badge Ctrl
     */
    function OriginAchievementBadgeCtrl($scope, UtilFactory, moment) {
        var hiddenPoints = "??";

        if (Number($scope.date) === -1) {
            // A date of -1 indicates that the achievement has not been completed
            $scope.notCompletedLoc = UtilFactory.getLocalizedStr($scope.notCompletedStr, CONTEXT_NAMESPACE, 'notcompleted');
            $scope.completedOnText = $scope.notCompletedLoc;
        } else {
            // Completion date is in unix epoch time format
            var date = new Date(0);
            date.setUTCSeconds($scope.date);
            $scope.completedOnLoc = UtilFactory.getLocalizedStr($scope.completedOnStr, CONTEXT_NAMESPACE, 'completedon', {
                '%date%': moment(date).format('LL')
            });
            $scope.completedOnText = $scope.completedOnLoc;
        }

        $scope.getPoints = function() {
            return $scope.points === "--" ? hiddenPoints : $scope.points;
        };
    }

    /**
     * Achievement Badge Directive
     */
    function originAchievementBadge(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                points: '@',
                imageURL: '@imageurl',
                description: '@',
                date: '@',
                notCompletedStr: '@notcompleted',
                completedOnStr: '@completedon'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('achievements/views/achievementbadge.html'),
            controller: 'OriginAchievementBadgeCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAchievementBadge
     * @restrict E
     * @scope
     * @param {string} title-str achievement title
     * @param {string} points - origin points awarded by the achievement
     * @param {string} description - achievement description
     * @param {string} date - the date that the achievement was completed in unix epoch time. -1 if not completed yet.
     * @param {ImageUrl} imageurl - URL of the badge image
     * @param {LocalizedString} notcompleted Not yet completed
     * @param {LocalizedString} completedon Completed text with date formated in MM d YYYY
     * @description
     *
     * Achievement Badge - displays info for a single achievement
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-achievement-badge
                   title="Driven"
                   points="5"
                   imageURL="http://some.server.path/achieve.jpg"
                   description="Return to active duty."
                   date="1456788246">
               </origin-achievement-badge>
     *     </file>
     * </example>
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginAchievementBadgeCtrl', OriginAchievementBadgeCtrl)
        .directive('originAchievementBadge', originAchievementBadge);
}());