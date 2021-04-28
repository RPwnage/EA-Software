/**
 * @file cqtargetingstatechange.js
 */
(function() {
    'use strict';

    var TEN_HOURS_MS = (1000 * 60 * 60 * 10); // 10 Hours in milliseconds;

    function CQTargetingStateChangeFactory($state, $timeout, DateHelperFactory, AppCommFactory) {

        var stateChangeInterval,
            stateChangeTimeOutPromise,
            myEvents = new Origin.utils.Communicator();

        /**
         * return the difference in time between the incomingVal and the currentEpoc
         * @param  {number} incomingVal a time stamp in milliseconds since epoch
         * @return {number} the difference in ms
         */
        function dayDiffFromCurrent(incomingVal) {
            return (new Date(incomingVal) - Date.now());
        }

        /**
         * Check to see if we should show the tile. If the startdate has passed for tile
         * @param  {string} startDate start date for when the tile will be shown under MyHome
         * @return {boolean} true if start date has passed, false otherwise
         */
        function isPastStartTime(startDate) {
            return DateHelperFactory.isInThePast(new Date(startDate));
        }

        /**
         * Check to see if we should hide the tile. If the enddate has passed for tile
         * @param  {string} endDate end date for when the tile will be removed from MyHome
         * @return {boolean} true if end date has passed, false otherwise
         */
        function isBeforeEndTime(endDate) {
            return DateHelperFactory.isInTheFuture(new Date(endDate));
        }

        /**
         * Clears out the state change timer and clear the interval and timeout promise.
         */
        function stopStateChangeTimer() {
            if (stateChangeTimeOutPromise) {
                $timeout.cancel(stateChangeTimeOutPromise);
                stateChangeInterval = undefined;
                stateChangeTimeOutPromise = null;
            }
        }

        /**
         * Fires the state change event for factory to show refresh this page banner
         */
        function suggestStateChange() {
            //let everyone know who cares that we should show the refresh page link as we have a new tile to show or hide
            myEvents.fire('CQTargetingStateChangeFactory:stateChanged');
        }

        /**
         * Update the state change timer to the set inteval and cancels out existing timers
         * @param  {string} interval interval in milliseconds
         */
        function updateStateChangeTimer(interval) {
            //stop the running timer if there is one
            stopStateChangeTimer();

            //kick off a timer
            stateChangeTimeOutPromise = $timeout(suggestStateChange, interval, false);
        }

        /**
         * Set the valid interval, checks if the new interval is less the existing one.
         * @param  {string} newInterval new interval in milliseconds
         */
        function setValidInterval(newInterval) {
            // We will only set one timer per home page so we are checking here to see if the new interval is less than the current one
            // then set the timeout for the new interval.
            if(!stateChangeInterval || newInterval < stateChangeInterval) {
                stateChangeInterval = newInterval;
                updateStateChangeTimer(newInterval);
            }
        }

        /**
         * Set the valid interval, checks if the new interval is less the existing one.
         * @param  {string} newInterval new interval in milliseconds
         */
        function getValidTime(time) {
            var diff = dayDiffFromCurrent(time);
            // We need to cap the timer at 10 hours max, if the time diff is more than that we will not set the timer.
            if(diff < TEN_HOURS_MS) {
                setValidInterval(diff);
            }
        }

        /**
         * Set the valid interval, checks if the new interval is less the existing one.
         * @param {Date} startTime start time for the tile
         * @param {Date} endTime end time for the tile
         */
        function setStateChangeTimer(startTime, endTime) {
            // If we don't have one of these times set don't do anything.
            if(!startTime && !endTime) {
                return;
            }

            // If we have the starttime and it has not passsed the current time - use that to set the timer 
            // otherwise check for starttime has passed and end time is still active then use that to set the timer.
            if(!isPastStartTime(startTime)) {
                getValidTime(startTime);
            } else if(isPastStartTime(startTime) && isBeforeEndTime(endTime)) {
                getValidTime(endTime);
            }
        }

        // on state change clear out the timers 
        AppCommFactory.events.on('uiRouter:stateChangeStart', stopStateChangeTimer);

        return {
            /**
             * Set the valid interval, checks if the new interval is less the existing one.
             * @param {Date} startTime start time for the tile
             * @param {Date} endTime end time for the tile
             */
            setStateChangeTimer: setStateChangeTimer,

            /**
             * events for the factory
             */
            events: myEvents
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories['cq-targeting-state-change']Factory

     * @description
     *
     * Factory that sets timer to show the refresh this page banner
     */
    angular.module('origin-components')
        .factory('CQTargetingStateChangeFactory', CQTargetingStateChangeFactory);
}());