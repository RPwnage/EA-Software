/**
 * @file helpers/components-config-helper.js
 */
(function() {
    'use strict';

    var HELP_LOCALE_MAPPING = {
        'da_DK': 'dk',
        'da': 'dk',
        'de_DE': 'de',
        'de': 'de',
        'en_US': 'en',
        'en': 'en',
        'es_MX': 'mx',
        'es_ES': 'es',
        'es': 'es',
        'fi_FI': 'fi',
        'fi': 'fi',
        'fr_CA': 'ca-fr',
        'fr_FR': 'fr',
        'fr': 'fr',
        'it_IT': 'it',
        'it': 'it',
        'ja_JP': 'jp',
        'ja': 'jp',
        'ko_KR': 'kr',
        'ko': 'kr',
        'nl_NL': 'nl',
        'nl': 'nl',
        'no_NO': 'no',
        'no': 'no',
        'pl_PL': 'pl',
        'pl': 'pl',
        'pt_BR': 'br',
        'pt': 'br',
        'ru_RU': 'ru',
        'ru': 'ru',
        'sv_SE': 'se',
        'sv': 'se',
        'th_TH': 'th',
        'th': 'th',
        'zh_TW': 'hk',
        'zh': 'hk'
    };

    var CART_NAME_WEB_KEY = 'cartNameWeb';
    var CART_NAME_CLIENT_KEY = 'cartNameClient';
    var cachedCartName;

    function ComponentsConfigHelper(UtilFactory, ComponentsLogFactory, COMPONENTSCONFIG) {

        function init() {
            Origin.utils.replaceTemplatedValuesInConfig(COMPONENTSCONFIG);

            ComponentsLogFactory.info('COMPONENT URLS:', COMPONENTSCONFIG.urls);

        }

        /**
         * Get a setting from the appropriate config file
         * @return {string} The requested config
         */
        function getSetting(id) {
            return COMPONENTSCONFIG.settings[id];
        }
        /**
         * Get a setting from the appropriate config file
         * @return {string} The requested config
         */
        function getAllSettings() {
            return COMPONENTSCONFIG.settings;
        }

        function getUrl(id) {
            return COMPONENTSCONFIG.urls[id];
        }

        function getHelpUrl(locale, lang) {
            var helpUrl = COMPONENTSCONFIG.urls.help,
                helpLocale = HELP_LOCALE_MAPPING[locale];

            if (_.isUndefined(helpLocale)) {
                if (!_.isUndefined(HELP_LOCALE_MAPPING[lang])) {
                    helpLocale = HELP_LOCALE_MAPPING[lang];
                } else {
                    helpLocale = 'en';
                }
            }

            return helpUrl.replace('{locale}', helpLocale);
        }

        function getAutomationConfig() {
            return COMPONENTSCONFIG.overrides ? COMPONENTSCONFIG.overrides.automation : undefined;
        }

        function getHostnameConfigs() {
            return COMPONENTSCONFIG.hostname;
        }

        function getCheckoutConfigs() {
            return COMPONENTSCONFIG.checkout;
        }

        function getEnableSocialConfig() {
            return COMPONENTSCONFIG.overrides ? COMPONENTSCONFIG.overrides.enablesocial : undefined;
        }

        function getHideSiteStripeConfig() {
            return COMPONENTSCONFIG.overrides ? COMPONENTSCONFIG.overrides.hidesitestripe : undefined;
        }

        function getOIGContextConfig() {
            return COMPONENTSCONFIG.overrides ? COMPONENTSCONFIG.overrides.oigcontext : undefined;
        }

        function getVersion() {
            return COMPONENTSCONFIG.overrides ? COMPONENTSCONFIG.overrides.version : undefined;
        }

        function isOIGContext() {
            return angular.isDefined(getOIGContextConfig());
        }

        function enableSocialFeatures() {
        // We only want to enable social if we are in the embedded client, or there is an override, we are not an OIG window and the user is not under age
            return (Origin.client.isEmbeddedBrowser() || getEnableSocialConfig() === 'true') && !isOIGContext() && !Origin.user.underAge();
        }

        function getCartName() {
            var template, timestamp, cartName;
            if(Origin.client.isEmbeddedBrowser()) {
                cartName = getSetting(CART_NAME_CLIENT_KEY);
            } else if(!cachedCartName) {
                template = getSetting(CART_NAME_WEB_KEY);
                timestamp = (new Date()).getTime().toString();
                cachedCartName = cartName = template.replace(/({timestamp})/g, timestamp);
            } else {
                cartName = cachedCartName;
            }
            return cartName;
        }

        return {
            init: init,
            /**
             * returns the set of component service urls we should be using
             * @return {Promise} that resolves with the current set of componentUrls
             */
            getHostnameConfigs: getHostnameConfigs,
            getUrl: getUrl,
            getSetting: getSetting,
            getAllSettings: getAllSettings,
            getAutomationConfig: getAutomationConfig,
            getCheckoutConfigs: getCheckoutConfigs,
            getEnableSocialConfig: getEnableSocialConfig,
            getHideSiteStripeConfig: getHideSiteStripeConfig,
            getOIGContextConfig: getOIGContextConfig,
            getHelpUrl: getHelpUrl,
            getVersion: getVersion,
            isOIGContext: isOIGContext,
            enableSocialFeatures: enableSocialFeatures,
            getCartName: getCartName
        };

    }


    /**
     * @ngdoc service
     * @name origin-components.factories.ComponentsConfigHelper
     * @requires $http
     * @description
     *
     * ComponentsConfigHelper
     */
    angular.module('origin-components')
        .factory('ComponentsConfigHelper', ComponentsConfigHelper);

}());