(function() {
    'use strict';
    var locale = originLocaleApi.getLocale(),
        country = originLocaleApi.getThreeLetterCountryCode(),
        path = window.location.pathname;

    if (path.indexOf(locale) > -1) {
        document.getElementsByTagName('base')[0].href = '/' + country + '/' + locale + '/';
    }
}());
