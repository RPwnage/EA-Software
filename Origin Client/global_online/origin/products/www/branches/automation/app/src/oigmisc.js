/**
* miscellaneous individual JS that is included so it can be included in vendor
* @file oigmisc.js
*/
    /*globals moment */
    var locale = window.opener.OriginLocale.parse(window.opener.location.href, 'en-us').getLocale();
    if (locale === 'no-no') {
        locale = 'nn-no';
    }
    moment.locale(locale);
