/**
 * @file game/violator/timer.js
 */
(function() {

    'use strict';

    /* jshint ignore:start */
    /**
     * TimerTypeEnumeration
     * @enum {string}
     */
    var TimerTypeEnumeration = {
        "trial": "trial",
        "precise": "precise"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-game-violator-timer';

    function OriginGameViolatorTimerCtrl($scope, $interval, DateHelperFactory, UtilFactory, ScopeHelper) {
        var updatePromise;

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

            ScopeHelper.digestIfDigestNotInProgress($scope);
        }

        /**
         * Get the textual version of the countdown timer for trials
         * @return {string} the text string
         */
        function getTextTrial() {
            var timeData = DateHelperFactory.getCountdownData(getEnddate()),
                orderedTimeUnits = ['days', 'hours', 'minutes'],
                tokenObject = {},
                timeUnitValue = 0;

            if(timeData) {
                //loop through days/hours/minutes and find the first non zero value. If found then break early.
                for (var i = 0, j = orderedTimeUnits.length; i < j; i++) {
                    timeUnitValue = timeData[orderedTimeUnits[i]];
                    if (timeUnitValue > 0) {
                        tokenObject['%' + orderedTimeUnits[i] + '%'] = timeUnitValue + 1;
                        var localizationKey = [orderedTimeUnits[i], 'trial'].join('');

                        return UtilFactory.getLocalizedStr($scope[localizationKey], CONTEXT_NAMESPACE, localizationKey, tokenObject, timeUnitValue + 1);
                    }
                }
            } else {
                return '';
            }
        }

        /**
         * Get the textual version of the countdown timer for precise timer
         * @return {string} the text string
         */
        function getTextPrecise() {
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
        }

        var timerTypeMap = {
                trial: {
                    getText: getTextTrial,
                    refreshInterval: 60000
                },
                precise: {
                    getText: getTextPrecise,
                    refreshInterval: 1000
                }
            },
            timerType  = ($scope.format && timerTypeMap[$scope.format]) ? timerTypeMap[$scope.format] : timerTypeMap.precise;

        $scope.getText = timerType.getText;
        updatePromise = $interval(updateTimeInterval, timerType.refreshInterval, 0, false);

        $scope.$on('$destroy', function() {
            $interval.cancel(updatePromise);
        });
    }

    function originGameViolatorTimer() {
        return {
            restrict: 'E',
            scope: {
                format: '@',
                enddate: '@',
                daysleft: '@',
                hoursleft: '@',
                minutesleft: '@',
                secondsleft: '@',
                daystrial: '@',
                hourstrial: '@',
                minutestrial: '@'
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
     * @param {TimerTypeEnumeration} format the timer format
     * @param {DateTime} enddate future date to count down (UTC iso date)
     * @param {LocalizedString} daysleft pluralized string for days left in an event date - use %days% to fill in the number of days in the string
     * @param {LocalizedString} hoursleft pluralized string for hours left in an event date - use %hours% to fill in the number of hours in the string
     * @param {LocalizedString} minutesleft pluralized string for minutes left in an event date - use %minutes% to fill in the number of minutes in the string
     * @param {LocalizedString} secondsleft pluralized string for seconds left in an event date - use %seconds% to fill in the number of seconds in the string
     * @param {LocalizedString} daystrial pluralized string for days left in a trial's play time - use %days% to fill in the number of days in the string
     * @param {LocalizedString} hourstrial pluralized string for hours left in a trial's play time - use %hours% to fill in the number of hours in the string
     * @param {LocalizedString} minutestrial pluralized string for minutes left in a trial's play time - use %minutes% to fill in the number of minutes in the string
     * @description
     *
     * Generic inline countdown timer that shows countdown in days > hours > minutes > seconds as natural language
     * The precise format is used in the case of fixed end dates where the time left is known to the seconds
     * The trial format is used in the case of game trials where the countdown is only approximate (usually within 5-10 mins)
     * (precise mode) Game expires in [30 seconds]
     * (trial mode) Game expires in [less than 5 minutes]
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-game-violator-timer
     *                   format="precise"
     *                   enddate="2016-04-05T24:00:00Z"
     *                   daysleft="{0} less than a day | {1} 1 day | ]1,+Inf] %days% days"
     *                   hoursleft="{0} less than an hour | {1} 1 hour | ]1,+Inf] %hours% hours"
     *                   minutesleft="{0} less than a minute | {1} 1 minute | ]1,+Inf] %minutes% minutes"
     *                   secondsleft="{1} %seconds% second | ]-1,+Inf] %seconds% seconds"
     *                   daystrial="less than %days% days"
     *                   hourstrial="]-1, 23] less than %hours% hours | {24} less than an day"
     *                   minutestrial="]-1, 5] less than 5 minutes |]5, 15] less than 15 minutes | ]15, 30] less than 30 minutes | ]30,+Inf] less than an hour">
     *             </origin-game-violator-timer>
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGameViolatorTimer', originGameViolatorTimer)
        .controller('OriginGameViolatorTimerCtrl', OriginGameViolatorTimerCtrl);
}());
