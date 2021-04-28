/*jshint strict: false */
/*jshint unused: false */

define([
    'core/events',
], function(events) {
    /**
     * Locale collection
     * @module module:locale
     * @memberof module:Origin
     */
    var locale = {
            'locale': {
                'value': 'en_US',
                'event': events.LOCALE_CHANGED
            },
            'languageCode': {
                'value': 'en',
                'event': events.LANGUAGE_CODE_CHANGED
            },
            'countryCode': {
                'value': 'US',
                'event': events.COUNTRY_CODE_CHANGED
            },
            'threeLetterCountryCode': {
                'value': 'USA',
                'event': events.THREE_LETTER_COUNTRY_CODE_CHANGED
            },
            'currencyCode': {
                'value': 'USD',
                'event': events.CURRENCY_CODE_CHANGED
            }
        },

        //map from cq5 locale to eadp locale by country, if entry is missing, then no need to map
        //table is set here: https://confluence.ea.com/pages/viewpage.action?spaceKey=EBI&title=Origin+X+EADP+Store+Front+Mappings
        locale2eadplocale = {
            'en_US': {
                'AU': 'en_AU',
                'BE': 'en_BE',
                'BR': 'en_BR',
                'CA': 'en_CA',
                'DE': 'en_DE',
                'DK': 'en_DK',
                'ES': 'en_ES',
                'FI': 'en_FI',
                'FR': 'en_FR',
                'GB': 'en_GB',
                'HK': 'en_HK',
                'IE': 'en_IE',
                'IN': 'en_IN',
                'IT': 'en_IT',
                'JP': 'en_JP',
                'KR': 'en_KR',
                'MX': 'en_MX',
                'NL': 'en_NL',
                'NO': 'en_NO',
                'NZ': 'en_NZ',
                'PL': 'en_PL',
                'RU': 'en_RU',
                'SE': 'en_SE',
                'SG': 'en_SG',
                'TH': 'en_TH',
                'TW': 'en_TW',
                'ZA': 'en_ZA'
            },

            'fr_FR': {
                'BE': 'fr_BE'
            },

            'nl_NL': {
                'BE': 'nl_BE'
            }
        };



    /**
     * Set a local value if the input value is different and not falsy
     * @param {string} key   The collection key name
     * @param {string} value The value to set
     */
    function set(key, value) {
        if(!value || locale[key].value === value) {
            return;
        }

        locale[key].value = value;
        events.fire(locale[key].event);
    }

    /**
     * Get a value
     * @param  {string} key the key to look up
     * @return {string} the value
     */
    function get(key) {
        return locale[key].value;
    }

    /**
     * set the locale
     * @param {string} locale in the format of locale_country eg. en_US
     */
    function setLocale(locale) {
        set('locale', locale);
    }

    /**
     * set the language code
     * @param {string} the ISO_3166-1 alpha 2 upper cased language code eg. en
     */
    function setLanguageCode(languageCode) {
        set('languageCode', languageCode);
    }

    /**
     * set the country code
     * @param {string} the ISO_3166-1 alpha 2 upper cased country code eg. US
     */
    function setCountryCode(countryCode) {
        set('countryCode', countryCode.toUpperCase());
    }

    /**
     * set the three letter country code
     * @param {string} the ISO_3166-1 alpha 3 upper cased country code eg. USA
     */
    function setThreeLetterCountryCode(threeLetterCountryCode) {
        set('threeLetterCountryCode', threeLetterCountryCode.toUpperCase());
    }

    /**
     * set the currency code
     * @param {string} the three letter currency code eg. USD
     */
    function setCurrencyCode(currencyCode) {
        set('currencyCode', currencyCode.toUpperCase());
    }

    /**
     * retrieve the locale
     * @return {string} locale in the format of locale_country eg. en_US
     */
    function getLocale(locale) {
        return get('locale');
    }

    /**
     * retrieve the language code
     * @return {string} the ISO_3166-1 alpha 2 upper cased language code eg. en
     */
    function getLanguageCode(languageCode) {
        return get('languageCode');
    }

    /**
     * retrieve the country code
     * @return {string} the ISO_3166-1 alpha 2 upper cased country code eg. US
     */
    function getCountryCode(countryCode) {
        return get('countryCode');
    }

    /**
     * Retrieve the 3 letter country code
     * @return {string} the ISO_3166-1 alpha 3 upper cased country code eg. USA
     */
    function getThreeLetterCountryCode(threeLetterCountryCode) {
        return get('threeLetterCountryCode');
    }

    /**
     * Retrive the currency code
     * @return {string} the ISO_4217  currency code eg. USD
     */
    function getCurrencyCode(currencyCode) {
        return get('currencyCode');
    }

    /**
     * EADP locale differs from CQ5 locales so we need to remap it, e.g. en-us.CAN => en-ca.CAN
     * @param {string} language 2-letter language, e.g. en
     * @param {string} country 2-letter country code
     * @return {string} eadpLocale locale to be used for catalog
     */
    function getEADPlocale(language, country) {
        var eadpLocale = language;

        if (locale2eadplocale[language] && locale2eadplocale[language][country]) {
            eadpLocale = locale2eadplocale[language][country];
        }
        return eadpLocale;
    }

    //we expose this as Origin.locale in jssdk.js
    return /** @lends module:Origin.module:locale */{
        /**
         * set the locale
         * @param {string} locale in the format of locale_country eg. en_US
         * @method
         */
        setLocale: setLocale,

        /**
         * set the language code
         * @param {string} the ISO_3166-1 alpha 2 upper cased language code eg. en
         * @method
         */
        setLanguageCode: setLanguageCode,

        /**
         * set the country code
         * @param {string} the ISO_3166-1 alpha 2 upper cased country code eg. US
         * @method
         */
        setCountryCode: setCountryCode,

        /**
         * set the three letter country code
         * @param {string} the ISO_3166-1 alpha 3 upper cased country code eg. USA
         * @method
         */
        setThreeLetterCountryCode: setThreeLetterCountryCode,

        /**
         * set the country code
         * @param {string} the three letter currency code eg. USD
         * @method
         */
        setCurrencyCode: setCurrencyCode,

        /**
         * retrieve the locale
         * @return {string} locale in the format of locale_country eg. en_US
         * @method
         */
        locale: getLocale,

        /**
         * EADP locale differs from CQ5 locales so we need to remap it, e.g. en-us.CAN => en-ca.CAN
         * @param {string} language 2-letter language, e.g. en
         * @param {string} country 3-letter country code
         * @return {string} eadpLocale locale to be used for catalog
         */
        eadpLocale: getEADPlocale,

        /**
         * retrieve the language code
         * @return {string} the ISO_3166-1 alpha 2 upper cased language code eg. en
         * @method
         */
        languageCode: getLanguageCode,

        /**
         * retrieve the country code
         * @return {string} the ISO_3166-1 alpha 2 upper cased country code eg. US
         * @method
         */
        countryCode: getCountryCode,

        /**
         * Retrieve the 3 letter country code
         * @return {string} the ISO_3166-1 alpha 3 upper cased country code eg. USA
         * @method
         */
        threeLetterCountryCode: getThreeLetterCountryCode,

        /**
         * Retrive the currency code
         * @return {string} the ISO_4217  currency code eg. USD
         * @method
         */
        currencyCode: getCurrencyCode
    };
});