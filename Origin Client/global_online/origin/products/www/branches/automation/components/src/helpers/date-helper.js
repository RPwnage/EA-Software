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
            MILLISECONDS_IN_SECOND = 1000,
            DEFAULT_TIMEZONE_DATE_FORMAT = 'LLL z';

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
         * Subtract a number of days from a date field - makes a copy of the date so as not to mutate
         * a cached object
         * @param {Object} date A date Object
         * @param {Number} days the number of days to subtract
         * @return {Object} date object
         */
        function subtractDays(date, days) {
            if (!isValidDate(date)) {
                return;
            }

            var newDate = new Date(date.getTime());
            newDate.setHours(newDate.getHours() - (days * 24));

            return newDate;
        }

        /**
         * Get a breakdown of days, hours, minutes and seconds based on a specified remaining time and return it as an object
         * This can be used to power the countdown timer functionality in the site
         * @param {number} elapsed amount of time remaining in milliseconds
         * @return {Object} an object containing the number of days, hours, minutes and seconds left before the future date
         */
        function getCountdownDataFromTimeRemaining(elapsed) {
            var result = {};

            if (elapsed < 0) {
                elapsed = 0;
            }

            result.totalMillis = elapsed;

            result.totalSeconds = Math.floor(elapsed / MILLISECONDS_IN_SECOND);

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

            return getCountdownDataFromTimeRemaining(elapsed);
        }

        /**
         * Compare two dates and return true if the are the same
         * @param {Date} date1
         * @param {Date} date2
         * @return {Boolean}
         */
        function isSameDate(date1, date2) {
            if(isValidDate(date1) && isValidDate(date2)) {
                return date1.getTime() === date2.getTime();
            }

            return false;
        }

        /**
         * Ensure that the current date/time is between a start and end date/time
         * @param {Date} startDate the start date
         * @param {Date} endDate the end date
         * @return {Boolean}
         */
        function isBetweenDates(startDate, endDate) {
            return isInThePast(startDate) && isInTheFuture(endDate);
        }


        /**
         * Format a localized date string with timezone information
         * @param {Date} date a plain old javascript date object local times will be internally converted to UTC first
         * @param {Object} options provides overrides for timezone and formatting
         * @return {String} the formatted date to display
         * @see http://momentjs.com/docs/#/i18n/changing-locale/
         * @see http://momentjs.com/timezone/docs/#/using-timezones/formatting/
         */
        function formatDateWithTimezone(date, options) {
            var timezone = _.get(options, 'timezone') || moment.tz.guess(),
                format = _.get(options, 'format') || DEFAULT_TIMEZONE_DATE_FORMAT;

            if(!isValidDate(date)) {
                return '';
            }

            return moment.utc(date)
                    .tz(timezone)
                    .format(format);
        }

        return {
            isValidDate: isValidDate,
            isInTheFuture: isInTheFuture,
            isInThePast: isInThePast,
            addDays: addDays,
            subtractDays: subtractDays,
            getCountdownData: getCountdownData,
            getCountdownDataFromTimeRemaining: getCountdownDataFromTimeRemaining,
            isSameDate: isSameDate,
            isBetweenDates: isBetweenDates,
            formatDateWithTimezone: formatDateWithTimezone
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