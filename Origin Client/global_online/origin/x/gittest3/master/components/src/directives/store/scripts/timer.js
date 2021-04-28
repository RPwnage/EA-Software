/**
` * @file store/timer.js
 */
(function(){
    'use strict';
    /* jshint ignore:start */
    var ExpiredDateTime = {
        "format" : "F j Y H:i:s T"
    };
    /* jshint ignore:end */
    var DAYS_IN_YEAR = 365;
    var HOURS_IN_DAY = 24;
    var MINUTES_IN_HOUR = 60;
    var SECONDS_IN_MINUTE = 60;
    var MILLISECONDS_IN_SECOND = 1000;

    function OriginStoreTimerCtrl($scope) {
        /**
         * Check to see if the goal date has been reached
         *
         * @return {boolean} Returns true is the goal date has been reached or passed.
         */
        $scope.goalAchieved = function (){
            return ((new Date($scope.goalDate)).getTime() - (new Date()).getTime() <= 0);
        };
    }


    function asTimeStamp(date) {
        var goalTime = new Date(date);
        var currentTime = new Date();
        return goalTime.getTime() - currentTime.getTime();
    }

    function dateToArray(date) {
        var elapsed = asTimeStamp(date);
        var result = {};

        result.years = Math.floor(
            elapsed /
            (SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY * DAYS_IN_YEAR * MILLISECONDS_IN_SECOND)
        );

        elapsed = elapsed % (
            SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY  * DAYS_IN_YEAR * MILLISECONDS_IN_SECOND
        );

        result.days = Math.floor(
            elapsed /
            (SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY * MILLISECONDS_IN_SECOND)
        );

        elapsed = elapsed % (
            SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY * MILLISECONDS_IN_SECOND
        );
        result.hours = Math.floor(
            elapsed /
            (SECONDS_IN_MINUTE * MINUTES_IN_HOUR * MILLISECONDS_IN_SECOND)
        );

        elapsed = elapsed % (
            SECONDS_IN_MINUTE * MINUTES_IN_HOUR * MILLISECONDS_IN_SECOND
        );

        result.minutes = Math.floor(
            elapsed /
            (SECONDS_IN_MINUTE * MILLISECONDS_IN_SECOND)
        );

        result.seconds = Math.floor((elapsed / MILLISECONDS_IN_SECOND) % SECONDS_IN_MINUTE);
        return result;
    }

    /**
    * The directive
    */
    function originStoreTimer(ComponentsConfigFactory, $interval) {
        /**
        * The directive link
        */
        function OriginStoreTimerLink(scope) {
            // Add Scope functions and call the controller from here
            scope.time = {};

            /**
             * Update the scopes count down to the current countdown.
             *
             * @return {void}
             */
            scope.updateTime = function() {
                if(scope.goalAchieved()) {
                    $interval.cancel(scope.updatePromise);
                    scope.$emit('timerReached', {});
                    setTimeDigits();
                } else{
                    setTimeDigits();
                }
            };

            function setTimeDigits() {
                var time = dateToArray(scope.goalDate);
                for(var unit in time) {
                    if(time.hasOwnProperty(unit)){
                        if(time[unit] < 0) {
                            time[unit] = 0;
                        }
                        scope.time[unit]= formatNumber(time[unit]);
                    }
                }
            }

            function formatNumber(number) {
                var string = number.toString();
                if (string.length < 2) {
                    string = '0' + string;
                }
                return string;
            }

            //  Update the scope in interval
            scope.updatePromise = $interval(scope.updateTime, 1000);

            // fetch the timer data from the parent class.
            scope.$emit('fetchTimerData', {});
        }

        return {
            restrict: 'E',
            scope: {
                goalDate: '@'
            },
            link: OriginStoreTimerLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/timer.html'),
            controller: 'OriginStoreTimerCtrl'
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreTimer
     * @restrict E
     * @element ANY
     * @scope
     * @param {ExpiredDateTime} goal-date timer expire date
     *
     * Generic timer that can be added to components
     *
     */
    angular.module('origin-components')
        .directive('originStoreTimer', originStoreTimer)
        .controller('OriginStoreTimerCtrl', OriginStoreTimerCtrl);
}());
