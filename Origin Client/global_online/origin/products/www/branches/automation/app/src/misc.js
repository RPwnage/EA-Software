/**
* miscellaneous individual JS that is included so it can be included in vendor
* @file misc.js
*/
    /*globals moment */
    var locale = originLocaleApi.getLocale();
    // fix the locale for Norwegian as momentjs uses a different language code.
    if (locale === 'no-no') {
        locale = 'nn-no';
    }
    moment.locale(locale);
