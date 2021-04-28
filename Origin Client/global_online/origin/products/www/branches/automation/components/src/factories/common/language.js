/**
 * @file factories/common/language.js
 */
(function() {
    'use strict';

    function LanguageFactory(COMPONENTSCONFIG, $http, $cookies, UrlHelper, ComponentsLogFactory, STOREFRONT_TO_PRIMARY_LOCALES_MAPPING) {

        /**
         * Handle the http request
         * @param {XMLHttpRequest} the xhr response
         * @return {Object} a list of all languages
         * @method handleLanguageRequest
         */
        function handleLanguageRequest(response) {
            return response.data;
        }

        /**
         * Handle the http request error
         * @param {Error} the xhr error
         * @return {Object} empty object
         * @method handleLanguageFailure
         */
        function handleLanguageFailure(error) {
            ComponentsLogFactory.error('[LanguageFactory:getAllLanguages] Could not retrieve all languages', error);
            return {};
        }

        /**
         * Get all languages
         * @return {Promise} a list of all languages
         * @method getAllLanguages
         */
        function getAllLanguages() {
            var url =  COMPONENTSCONFIG.urls.languages;
            return $http.get(url)
                .then(handleLanguageRequest, handleLanguageFailure);
        }

        /**
        * Get the dashed locale
        * @return {String} dashed locale
        * @method getDashedLocale
        */
        function getDashedLocale() {
            return Origin.locale.locale().toLowerCase().replace('_', '-');
        }

        /**
        * Update the current locale and store it in the cookie
        * and reload the page
        * @param {String} locale
        * @method updateLocale
        */
        function setLocale(locale) {

            var currentLocale = getDashedLocale();
            if (locale && locale !== currentLocale) {

                // note that for now we are setting the domain to
                // .origin.com so that this value works across subdomains
                // but this means that the locale cookie will be passed
                // with every request to every subdomain which isn't great
                // for performance.  We should change this to window.location.domain
                // when we are looking at performance improvements.
                $cookies.put('locale', locale, {
                    path: '/',
                    domain: '.origin.com'
                });

                // the client will read the cookie and automatically
                // reload the client when it changes.
                if (!Origin.client.isEmbeddedBrowser()) {
                    window.location.replace(UrlHelper.buildLocaleUrl(locale));
                }
            }
        }

        function getPrimaryLocalesByStorefront(){
            var currentStorefront = Origin.locale.threeLetterCountryCode().toLowerCase();
            return STOREFRONT_TO_PRIMARY_LOCALES_MAPPING[currentStorefront] || STOREFRONT_TO_PRIMARY_LOCALES_MAPPING.default;
        }

        return {
            getAllLanguages: getAllLanguages,
            getDashedLocale: getDashedLocale,
            setLocale: setLocale,
            getPrimaryLocalesByStorefront: getPrimaryLocalesByStorefront
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.LanguageFactory

     * @description
     *
     * LanguageFactory
     */
    angular.module('origin-components')
        .constant('STOREFRONT_TO_PRIMARY_LOCALES_MAPPING', {
            'aus': ['en-us'],
            'bel': ['fr-fr', 'nl-nl'],
            'bra': ['pt-br'],
            'can': ['en-us', 'fr-ca'],
            'deu': ['de-de'],
            'dnk': ['da-dk'],
            'esp': ['es-es'],
            'fin': ['fi-fi'],
            'fra': ['fr-fr'],
            'gbr': ['en-us'],
            'hkg': ['zh-hk', 'en-us'],
            'irl': ['en-us'],
            'ind': ['en-us'],
            'ita': ['it-it'],
            'jpn': ['ja-jp'],
            'kor': ['ko-kr'],
            'mex': ['es-mx'],
            'nld': ['nl-nl'],
            'nor': ['no-no'],
            'nzl': ['en-us'],
            'pol': ['pl-pl'],
            'rus': ['ru-ru'],
            'swe': ['sv-se'],
            'sgp': ['en-us'],
            'tha': ['th-th'],
            'twn': ['zh-tw'],
            'usa': ['en-us'],
            'zaf': ['en-us'],
            'default': ['en-us']
        })
        .factory('LanguageFactory', LanguageFactory);
}());
