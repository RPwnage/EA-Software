/**
` * @file store/timer.js
 */
(function(){

    'use strict';

    function OriginStoreTimerCtrl($scope, $interval, DateHelperFactory) {
        /**
         * Check to see if the goal date has been reached
         * @return {boolean} Returns true if the goal date has been reached or passed.
         */
        function goalAchieved() {
            return DateHelperFactory.isInThePast(new Date($scope.goalDate));
        }

        /**
         * Fetch the countdown data for the scope goal date and updae scope.time
         */
        function updateCountdownData() {
            var countdownData = DateHelperFactory.getCountdownData(new Date($scope.goalDate));

            if(countdownData) {
                $scope.time = countdownData;
            }
        }

        /**
         * Update the scopes count down to the current countdown. If the countdown timer
         * reaches zero, $emit a 'timerReached' event so that another component can react to the goal conditions being met
         */
        function updateTimeInterval(){
            if(goalAchieved()) {
                $interval.cancel($scope.updatePromise);
                $scope.$emit('timerReached');
            }

            updateCountdownData();
        }

        updateTimeInterval();
        $scope.updatePromise = $interval(updateTimeInterval, 1000);
        $scope.$on('$destroy', function() {
            $interval.cancel($scope.updatePromise);
        });
    }

    function originStoreTimer(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                goalDate: '@'
            },
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
     * @param {DateTime} goal-date timer expire date
     * @description
     *
     * Generic table based countdown timer that can be added to components
     */
    angular.module('origin-components')
        .directive('originStoreTimer', originStoreTimer)
        .controller('OriginStoreTimerCtrl', OriginStoreTimerCtrl);
}());
