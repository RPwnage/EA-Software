/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
    'core/events'
], function (events) {

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

    function setEnvironment(newValue) {
        jssdkEnvironment = newValue;
    }

    function getEnvironment() {
        return jssdkEnvironment;
    }

    return {
        setLocale: setLocale,
        locale: getLocale,
        countryCode: getCountryCode,
        setEnvironment: setEnvironment,
        environment: getEnvironment,
    };
});
