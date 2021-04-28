/*jshint strict: false */
/*jshint unused: false */

define([
    'core/events'
], function(events) {
    /**
     * Contains locale and environment related functions
     * @module module:locale
     * @memberof module:Origin
     */
    var jssdkEnvironment;
    var jssdkLocale = 'en_US';

    function setLocale(locale) {
        if (typeof locale === 'undefined') {
            return;
        }

        if (jssdkLocale !== locale) {
            jssdkLocale = locale;
            //locale changed, emit a signal that it's changed
            events.fire(events.LOCALE_CHANGED);
        }
    }

    function getLocale() {
        return jssdkLocale;
    }

    function getCountryCode() {
        var loc = getLocale(),
            locSplit,
            countryCode = '';

        if (loc.length > 0) {
            locSplit = loc.split('_');
            if (locSplit.length > 1) {
                countryCode = locSplit[1];
            }
        }
        return countryCode;
    }


    //we expose this as Origin.locale in jssdk.js
    return /** @lends module:Origin.module:locale */{
        /**
         * set the locale
         * @param locale - in the format of locale_country (e.g. en_US)
         * @method
         */
        setLocale: setLocale,

        /**
         * retrieve the locale
         * @return {string} responsename locale in the format of locale_country (e.g. en_US)
         * @method
         */
        locale: getLocale,

        /**
         * retrieve the country code
         * @return {string} responsename the two letter country code extracted from locale
         * @method
         */
        countryCode: getCountryCode,
    };
});