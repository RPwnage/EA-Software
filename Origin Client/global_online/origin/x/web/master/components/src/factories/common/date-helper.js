/**
 * @file date-helper.js
 */
(function() {
    'use strict';

    function DateHelperFactory() {
        var DAYS_IN_YEAR = 365,
            HOURS_IN_DAY = 24,
            MINUTES_IN_HOUR = 60,
            SECONDS_IN_MINUTE = 60,
            MILLISECONDS_IN_SECOND = 1000;

        /**
         * Validate a javascript date object
         * @param {mixed} date the date to test
         * @return {Boolean} true if valid
         */
        function isValidDate(date){
            return (
                date &&
                Object.prototype.toString.call(date) === "[object Date]" &&
                !isNaN(date.getTime())
            ) ? true : false;
        }

        /**
         * Checks if the given date is in the future
         * @param {mixed} date date to check
         * @return {boolean}
         */
        function isInTheFuture(date) {
            return (isValidDate(date) && date > Date.now()) ? true : false;
        }

        /**
         * Checks if the given date now or in the past
         * @param {mixed} date date to check
         * @return {boolean}
         */
        function isInThePast(date) {
            return (isValidDate(date) && date <= Date.now()) ? true : false;
        }

        /**
         * Add a number of days to a date field - makes a copy of the date so as not to mutate
         * a cached object
         * @param {Object} date A date Object
         * @param {Number} days the number of days to add
         * @return {Object} date object
         */
        function addDays(date, days) {
            if (!isValidDate(date)) {
                return;
            }

            var newDate = new Date(date.getTime());
            newDate.setHours(newDate.getHours() + (days * 24));

            return newDate;
        }

        /**
         * Get a breakdown of days, hours, minutes and seconds between a given future date and now and return it as an object
         * This can be used to power the countdown timer functionality in the site
         * @param {Object} futureDate a javascript date object to compare with the current time
         * @param {Object} currentDate a javascript date object to fix the current time to a specified value (optional)
         * @return {Object} an object containing the number of days, hours, minutes and seconds left before the future date
         */
        function getCountdownData(futureDate, currentDate) {
            if (!isValidDate(futureDate)) {
                return;
            }

            if (currentDate && !isValidDate(currentDate)) {
                return;
            }

            currentDate = currentDate || new Date();
            var elapsed = futureDate.getTime() - currentDate.getTime();
            var result = {};

            if (elapsed < 0) {
                elapsed = 0;
            }

            elapsed = elapsed % (
                SECONDS_IN_MINUTE * MINUTES_IN_HOUR * HOURS_IN_DAY * DAYS_IN_YEAR * MILLISECONDS_IN_SECOND
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

        return {
            isValidDate: isValidDate,
            isInTheFuture: isInTheFuture,
            isInThePast: isInThePast,
            addDays: addDays,
            getCountdownData: getCountdownData
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function DateHelperFactorySingleton(moment, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('DateHelperFactory', DateHelperFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.DateHelperFactory
     * @description
     *
     * Date manipulation functions
     */
    angular.module('origin-components')
        .factory('DateHelperFactory', DateHelperFactorySingleton);
}());