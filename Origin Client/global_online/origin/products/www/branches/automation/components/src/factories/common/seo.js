/**
 * @file common/seo.js
 */
(function () {
    'use strict';
    var DEFAULT_PAGE_TITLE = 'Origin',
        NO_INDEX_FOLLOW = 'noindex, nofollow',
        HEAD_TAG = 'head',
        META_TAG = 'meta',
        META_NAME_ROBOTS = 'robots';

    /**
     * SEO factory communicate over a set of common interface with data format in the following
     * An array of objgma
     *
     * @constructor
     */
    function OriginSeoFactory($rootScope, OriginDomHelper, LanguageFactory) {
        var dataModel = [],
            metaTagUnsetter = {
                'title': OriginDomHelper.updateTag,
                'meta': OriginDomHelper.removeTag
            };
        /**
         * Unsets all SEO data that was previously set
         *
         * @param data
         * @returns {boolean}
         */
        function unsetAllSeoData() {
            _.forEach(dataModel, function(seoData) {
                var text = seoData.element === 'title' ? DEFAULT_PAGE_TITLE : '';
                var unsetter = metaTagUnsetter[seoData.element];
                if (unsetter){
                    unsetter('head', seoData.element, seoData.attrs, text);
                }
            });
            dataModel.length = 0;
            unsetRobotsRule();
        }

        /**
         * Set SEO data
         *
         * @param {string} element target element. E.g., meta
         * @param {Object} attrs key value pairs of attributes and their values
         * @param {string} text tag content
         */
        function setSeoData(element, attrs, text) {
            var textContent = element === 'title' ? (text || DEFAULT_PAGE_TITLE) : '';
            OriginDomHelper.updateTag('head', element, attrs, textContent);
            
            dataModel.push({
                element: element,
                attrs: attrs,
                text: text
            });
        }

        /**
         * Check if current locale is a standard locale for the current storefront
         * @returns {boolean} true if standard
         */
        function isDefaultLocale() {
            var primaryLocales = LanguageFactory.getPrimaryLocalesByStorefront();
            var currentLocale = LanguageFactory.getDashedLocale();
            return primaryLocales.indexOf(currentLocale) > -1;
        }

        /**
         * Sets meta tag name robots to the rule argument
         * @param rule
         */
        function setRobotsRule(rule) {
            OriginDomHelper.updateTag(HEAD_TAG, META_TAG, {
                name: META_NAME_ROBOTS,
                content: rule
            });
        }

        /**
         * Remove robots rule, only if we are browsing in a standard locale for the current storefront.
         */
        function unsetRobotsRule(){
            if (isDefaultLocale()) {
                OriginDomHelper.removeTag(HEAD_TAG, META_TAG, {
                    name: META_NAME_ROBOTS
                });
            }
        }

        /**
         * Set robots "noindex" rule if we are browsing in a non-standard locale
         */
        function setNoIndexRule(){
            if (!isDefaultLocale()){
                setRobotsRule(NO_INDEX_FOLLOW);
            }
        }

        /**
         * Needs to watch location changes success event instead of stateChangeSuccess event because of a UI router bug
         * This is to ensure that the un setting of SEO data, specifically the title, happens after AngularJS pushes the history object
         * onto the history stack
         * 
         * UI Router bug
         * https://github.com/angular-ui/ui-router/issues/273
         *
         */
        $rootScope.$on('$locationChangeSuccess', unsetAllSeoData);
        
        $rootScope.$on('$stateChangeSuccess', setNoIndexRule);

        return {
            setSeoData: setSeoData,
            setRobotsRule: setRobotsRule,
            unsetRobotsRule: unsetRobotsRule
        };
    }


    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OriginSeoFactorySingleton($rootScope, OriginDomHelper, LanguageFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OriginSeoFactory', OriginSeoFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OriginSeoFactory
     * @description
     *
     * Provide a common interface to access and interact with SEO data
     *
     */
    angular.module('origin-components')
        .factory('OriginSeoFactory', OriginSeoFactorySingleton);
})();
