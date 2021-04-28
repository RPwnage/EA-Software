/**
 *  * @file templateresolver.js
 */
(function () {
    'use strict';

    var COUNTRY_LOCALE_KEY = '{countrylocale}',
        FRANCHISE_KEY = '{franchise}',
        GAME_KEY = '{game}',
        TYPE_KEY = '{type}',
        EDITION_KEY = '{edition}',
        BUNDLE_KEY = '{bundle}',
        CATEGORY_PAGE_KEY = '{categorypageid}',
        CQ_PATH_KEY = '{cqPath}',
        NO_TEMPLATE_FOUND = '',
    //list of page templates that if not available shouldn't result in 404.
    //404 page itself should also be a part of the list, to prevent an infinite loop; i.e. when 404 page itself is not found
    //we don't want to app to continuously redirect to 404.
        WHITE_LIST = ['sitestripe', 'nux', 'oig-nux', 'footer-web', 'footer-client', 'not-found', 'global-sitestripe', 'oax-interstitial'],
        MAX_ERRORS = 3,
        NOT_FOUND_PAGE_ROUTE = 'app.error_notfound';


    function TemplateResolver($templateFactory, $http, $location, $rootScope, $state, LogFactory, APPCONFIG) {
        var redirectToErrorPage = 0;

        function getParam(param) {
            return param ? (param + '/') : '';
        }

        function buildTemplateUrl(templateKey, $stateParams) {
            var templateUrl = lookupTemplate(templateKey);

            if (templateUrl) {
                templateUrl = templateUrl
                    .replace(COUNTRY_LOCALE_KEY, getDefaultLocale())
                    .replace(FRANCHISE_KEY, getParam($stateParams.franchise))
                    .replace(GAME_KEY, getParam($stateParams.game))
                    .replace(TYPE_KEY, getParam($stateParams.type))
                    .replace(EDITION_KEY, $stateParams.edition || '')
                    .replace(BUNDLE_KEY, $stateParams.bundle || '')
                    .replace(CATEGORY_PAGE_KEY, $stateParams.categoryPageId || '')
                    .replace(CQ_PATH_KEY, $stateParams.cqPath || '');
            }
            return templateUrl;
        }

        function onStateChangeStart(event, toState) {
            if (toState.name !== 'app.error_notfound') {
                redirectToErrorPage = 0;
            }
        }

        function resetErrorCountOnStateChange() {
            $rootScope.$on('$stateChangeStart', onStateChangeStart);
        }

        function lookupTemplate(templateKey) {

            var templateUrl = APPCONFIG.urls[templateKey];

            if (!templateUrl) {
                LogFactory.error('Could not find template with key ' + templateKey);
            }

            return templateUrl;
        }

        /**
         * Determines whether the template a primary or secondary template. All pages except sitestripe are primary.
         * @param templateKey
         * @returns {boolean}
         */
        function isSecondaryTemplate(templateKey) {
            return WHITE_LIST.indexOf(templateKey) >= 0;
        }

        function loadTemplate(templateUrl, isSecondary, force) {
            if (force) {
                return $http.get(templateUrl)
                    .then(function (response) {
                        return response.data;
                    })
                    .catch(handleError(templateUrl, isSecondary));
            } else {
                return $templateFactory.fromUrl(templateUrl)
                    .catch(handleError(templateUrl, isSecondary));
            }

        }

        /**
         * Gets template from CQ5.
         * @param {string} templateKey key specified in tools/config/app-config.json
         * @param {Object} $stateParams angular service
         * @param {Boolean} force by default a template is cached. To force a fresh request, pass true
         * @returns {*}
         */
        function getTemplate(templateKey, $stateParams, force) {
            var templateUrl = buildTemplateUrl(templateKey, $stateParams);
            if (templateUrl) {
                return loadTemplate(templateUrl, isSecondaryTemplate(templateKey), force);
            } else {
                return '';
            }
        }

        function maxErrorNotReached() {
            return (redirectToErrorPage < MAX_ERRORS);
        }

        /**
         * @param templateUrl
         * @param isSecondary
         * @returns {Function}
         */
        function handleError(templateUrl, isSecondary) {
            return function () {
                LogFactory.error('Could not load template from url ' + templateUrl);

                if (!isSecondary) {

                    if (maxErrorNotReached()) {
                        redirectToErrorPage++;
                        return $state.go(NOT_FOUND_PAGE_ROUTE, $location.search(), {location: 'replace'});
                    } else {
                        return NO_TEMPLATE_FOUND;
                    }
                }
                return NO_TEMPLATE_FOUND;
            };
        }

        /**
         * OCD requires a unique locale format to other services in the format "en-gb.gbr"
         * @return {string} ocd's specific language code format for HTTP requests
         */
        function getDefaultLocale() {
            var urlSafeLocale = Origin.locale.locale().toLowerCase().replace('_', '-'),
                threeLetterCountryCode = Origin.locale.threeLetterCountryCode().toLowerCase();
            return [urlSafeLocale, threeLetterCountryCode].join('.');
        }


        resetErrorCountOnStateChange();

        return {
            getTemplate: getTemplate
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OriginTemplateProvider
     * @description
     *
     * TemplateResolver
     */
    angular.module('originApp')
        .factory('TemplateResolver', TemplateResolver);
}());
