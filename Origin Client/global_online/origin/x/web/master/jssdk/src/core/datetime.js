/*jshint strict: false */
define([], function() {
    /**
     * Contains date/time related methods
     * @module module:datetime
     * @memberof module:Origin
     */
    var trustedClock = Date.now(),
        trustedClockInitialized = false;

    function initializeTrustedClock(serverTime) {
        trustedClockInitialized = true;
        trustedClock = new Date(serverTime);
    }

    function secondsToDHMS(secs) {
        var timeObject = {};

        timeObject.days = Math.floor(secs / 86400);
        secs -= timeObject.days * 86400;
        timeObject.hours = Math.floor(secs / 3600) % 24;
        secs -= timeObject.hours * 3600;
        timeObject.minutes = Math.floor(secs/ 60) % 60;
        secs -= timeObject.minutes * 60;
        timeObject.seconds = secs % 60;
        return timeObject;
    }

    function getTrustedClock() {
        //until we can actually get a trusted clock, just return local time
        return Date.now();
    }

    return /** @lends module:Origin.module:datetime */ {
        /**
         * [initializeTrustedClock description]
         * @method
         * @static
         * @param {string} serverTime - sets the trusted clock based on servertime passed in
         */
        initializeTrustedClock: initializeTrustedClock,

        /**
         * [isInitialized description]
         * @method
         * @static
         * @return {boolean} returns true/false on whether the trusted clock was initialized to some server time
         */
        isInitialized: function () {
            return trustedClockInitialized;
        },

        /**
         * [getTrustedClock description]
         * @method
         * @static
         * @return {Date} returns the trust clock in Date
         */
        getTrustedClock: getTrustedClock,

        /**
         * @typedef timeObject
         * @type {object}
         * @property {number} days
         * @property {number} hours
         * @property {number} minutes
         * @property {number} seconds
         */

        /**
         * [secondsToDHMS description]
         * @method
         * @static
         * @param {number} secs  seconds
         * @return {timeObject} the seconds broken down into days, hours, minutes, seconds
         */
        secondsToDHMS: secondsToDHMS
    };
});