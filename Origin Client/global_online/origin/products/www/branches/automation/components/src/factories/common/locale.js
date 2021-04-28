/**
 * @file locale.js
 */
(function() {
    'use strict';
    
    var COUNTRIES = {
        'AU': {
            currency: 'AUD',
            ratingSystem: 'COB'
        },
        'BE': {
            currency: 'EUR',
            ratingSystem: 'PEGI'
        },
        'BR': {
            currency: 'BRL',
            ratingSystem: 'DJCTQ'
        },
        'CA': {
            currency: 'CAD',
            ratingSystem: 'ESRB'
        },
        'DE': {
            currency: 'EUR',
            ratingSystem: 'USK'
        },
        'DK': {
            currency: 'DKK',
            ratingSystem: 'PEGI'
        },
        'ES': {
            currency: 'EUR',
            ratingSystem: 'PEGI'
        },
        'FI': {
            currency: 'EUR',
            ratingSystem: 'PEGI'
        },
        'FR': {
            currency: 'EUR',
            ratingSystem: 'PEGI'
        },
        'GB': {
            currency: 'GBP',
            ratingSystem: 'PEGI'
        },
        'HK': {
            currency: 'HKD'
        },
        'IE': {
            currency: 'EUR',
            ratingSystem: 'PEGI'
        },
        'IN': {
            currency: 'INR'
        },
        'IT': {
            currency: 'EUR',
            ratingSystem: 'PEGI'
        },
        'JP': {
            currency: 'JPY',
            ratingSystem: 'CERO'
        },
        'KR': {
            currency: 'KRW',
            ratingSystem: 'GRB'
        },
        'MX': {
            currency: 'USD',
            ratingSystem: 'ESRB'
        },
        'NL': {
            currency: 'EUR',
            ratingSystem: 'PEGI'
        },
        'NO': {
            currency: 'NOK',
            ratingSystem: 'PEGI'
        },
        'NZ': {
            currency: 'NZD',
            ratingSystem: 'OFLC'
        },
        'PL': {
            currency: 'PLN',
            ratingSystem: 'PEGI'
        },
        'RU': {
            currency: 'RUB',
            ratingSystem: 'RGRS'
        },
        'SE': {
            currency: 'SEK',
            ratingSystem: 'PEGI'
        },
        'SG': {
            currency: 'SGD',
            ratingSystem: 'MDA'
        },
        'TH': {
            currency: 'THB'
        },
        'TW': {
            currency: 'TWD',
            ratingSystem: 'TWAR'
        },
        'US': {
            currency: 'USD',
            ratingSystem: 'ESRB'
        },
        'ZA': {
            currency: 'ZAR',
            ratingSystem: 'FPB'
        }
    },
    LANGUAGES = {
        "ar_SA": "العربية",
        "en_QA": "English (QA)",
        "en_US": "English (US)",
        "en_AU": "English (AU)",
        "nl_BE": "Nederlands",
        "fr_BE": "Français (BE)",
        "pt_BR": "Português (BR)",
        "en_CA": "English (CA)",
        "en_MX": "English (MX)",
        "fr_CA": "Français (CA)",
        "cs_CZ": "Český",
        "zh_CN": "中文（简体）",
        "da_DK": "Dansk",
        "de_DE": "Deutsch (DE)",
        "es_ES": "Español (ES)",
        "fi_FI": "Suomi",
        "fr_FR": "Français (FR)",
        "zh_HK": "中文（繁體）",
        "en_HK": "English",
        "hi_IN": "हिन्दी",
        "en_IE": "English (IE)",
        "it_IT": "Italiano",
        "ja_JP": "日本語",
        "ko_KR": "한국어",
        "hu_HU": "Magyar",
        "es_MX": "Español (MX)",
        "nl_NL": "Nederlands",
        "en_NZ": "English (NZ)",
        "no_NO": "Norsk",
        "de_AT": "Deutsch (AT)",
        "pl_PL": "Polski",
        "pt_PT": "Português (PT)",
        "ru_RU": "Русский язык",
        "sv_SE": "Svenska",
        "zh_TW": "中文（繁體）",
        "th_TH": "ภาษาไทย",
        "en_GB": "English (UK)",
        "en_WW": "English (WW)",
        "de_CH": "Deutsch (CH)",
        "it_CH": "Italiano (CH)",
        "fr_CH": "Français (CH)",
        "tr_TR": "Türkçe",
        "el_GR" : "Ελληνικά",
        "es_BR" : "Português Brazil",
        "zh_TH" : "",
        "es_BO" : "",
        "es_EC" : "",
        "es_US" : "Español (US)",
        "es_UY" : "",
        "en_TW" : "",
        "pt_IT" : "",
        "es_AR" : "",
        "es_NI" : "",
        "es_SV" : "",
        "es_CO" : "",
        "es_GT" : "",
        "es_CL" : "",
        "nn_NO" : "",
        "es_PE" : "",
        "es_CR" : "",
        "es_PA" : "",
        "es_HN" : "",
        "es_PR" : "",
        "es_DO" : "",
        "ar_AE" : "",
        "es_PY" : ""
    };

    function LocaleFactory() {

        function getProperty(countryCode, property) {
            if (!COUNTRIES.hasOwnProperty(countryCode)) {
                throw new Error('[LocaleFactory] Unknown country code: ' + countryCode);
            }

            return COUNTRIES[countryCode][property];
        }

        /**
         * Returns currency ID for the given country
         * @param {string} countryCode country code
         * @return {string}
         */
        function getCurrency(countryCode) {
            return getProperty(countryCode, 'currency');
        }

        /**
         * Returns rating system for the given country
         * @param {string} countryCode country code
         * @return {string}
         */
        function getRatingSystem(countryCode) {
            return getProperty(countryCode, 'ratingSystem');
        }

        /**
         * Takes a locale string and returns the language associated with that locale
         * @param  {string} locale locale to be found
         * @return {string} language string for the given locale or local key itself
         */
        function getLanguage(locale) {
            if(_.isString(locale)){
                var langCountry = locale.split('_');
                if (langCountry.length  === 2) {
                    var lang = langCountry[0].toLowerCase();
                    var country = langCountry[1].toUpperCase();
                    var key = lang + '_' + country;
                    var transLang = LANGUAGES[key];
                    return transLang || key;
                }
            }
            return locale; //return something
        }

        /**
         * Takes an array of locales and returns an array of associated languages for those locales (if they exist).
         * @param  {Array}  locales List of locales to be mapped
         * @return {Array} array of languages
         */
        function getLanguages(locales) {
            if (_.isArray(locales)){
                return _.compact(_.map(locales, getLanguage));
            }
        }

        return {
            getCurrency: getCurrency,
            getRatingSystem: getRatingSystem,
            getLanguage: getLanguage,
            getLanguages: getLanguages
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function LocaleFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('LocaleFactory', LocaleFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.LocaleFactory

     * @description
     *
     * Locale-specific constants
     */
    angular.module('origin-components')
        .factory('LocaleFactory', LocaleFactorySingleton);
    
    

}());