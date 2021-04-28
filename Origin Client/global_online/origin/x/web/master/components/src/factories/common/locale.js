/**
 * @file locale.js
 */
(function() {
    'use strict';

    function LocaleFactory() {
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
        };

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

        return {
            getCurrency: getCurrency,
            getRatingSystem: getRatingSystem
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