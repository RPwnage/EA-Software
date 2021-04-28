/**
 * @file game/violator/timer.js
 */
(function() {

    'use strict';

    function OriginGameViolatorTimerCtrl($scope, $interval, DateHelperFactory, UtilFactory) {
        var CONTEXT_NAMESPACE = 'origin-game-violator-timer',
            updatePromise;

        /**
         * Convert the end date to a javascript date object
         * @return {Object} a javascript date object
         */
        function getEnddate() {
            return new Date($scope.enddate);
        }

        /**
         * Check to see if the end date has been reached
         * @return {boolean} Returns true if the end date has been reached or passed.
         */
        function enddateReached() {
            return DateHelperFactory.isInThePast(getEnddate());
        }

        /**
         * Count down to the end date and cancel the interval when reached
         */
        function updateTimeInterval(){
            if(enddateReached()) {
                $interval.cancel(updatePromise);
            }
        }

        /**
         * Get the textual version of the countdown timer
         * @return {string} the text string
         */
        $scope.getText = function() {
            var countdownData = DateHelperFactory.getCountdownData(getEnddate()),
                token;

            if(countdownData) {
                if (countdownData.days > 0) {
                    token = {'%days%': countdownData.days};
                    return UtilFactory.getLocalizedStr($scope.daysleft, CONTEXT_NAMESPACE, 'daysleft', token, countdownData.days);
                } else if (countdownData.hours > 0) {
                    token = {'%hours%': countdownData.hours};
                    return UtilFactory.getLocalizedStr($scope.hoursleft, CONTEXT_NAMESPACE, 'hoursleft', token, countdownData.hours);
                } else if (countdownData.minutes > 0) {
                    token = {'%minutes%': countdownData.minutes};
                    return UtilFactory.getLocalizedStr($scope.minutesleft, CONTEXT_NAMESPACE, 'minutesleft', token, countdownData.minutes);
                } else {
                    token = {'%seconds%': countdownData.seconds};
                    return UtilFactory.getLocalizedStr($scope.secondsleft, CONTEXT_NAMESPACE, 'secondsleft', token, countdownData.seconds);
                }
            }

            return '';
        };

        updatePromise = $interval(updateTimeInterval, 1000);

        $scope.$on('$destroy', function() {
            $interval.cancel(updatePromise);
        });
    }

    function originGameViolatorTimer() {
        return {
            restrict: 'E',
            scope: {
                daysleft: '@daysleft',
                hoursleft: '@hoursleft',
                minutesleft: '@minutesleft',
                secondsleft: '@secondsleft',
                enddate: '@enddate'
            },
            controller: OriginGameViolatorTimerCtrl,
            template: '{{getText()}}'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorTimer
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} daysleft pluralized translation for days left
     * @param {LocalizedString} daysleft pluralized translation for hours left
     * @param {LocalizedString} daysleft pluralized translation for minutes left
     * @param {LocalizedString} daysleft pluralized translation for seconds left
     * @param {DateTime} enddate future date to count down - iso date or date object will work
     * @description
     *
     * Generic inline countdown timer that shows countdown in days > hours > minutes > seconds as lanugage
     * example: Game expires in [2 days]
     * example: Game Expires in [an hour]
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-game-violator-timer
     *                   daysleft = "{0} less than a day | {1} a day | ]1,+Inf] %days% days"
     *                   hoursleft = "{0} less than an hour | {1} an hour | ]1,+Inf] %hours% hours"
     *                   minutesleft: "{0} less than a minute | {1} 1 minute | ]1,+Inf] %minutes% minutes"
     *                   secondsleft = "{1} %seconds% second | ]-1,+Inf] %seconds% seconds"
     *                   enddate: "2008-05-11T15:30:00Z"
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGameViolatorTimer', originGameViolatorTimer)
        .controller('OriginGameViolatorTimerCtrl', OriginGameViolatorTimerCtrl);
}());