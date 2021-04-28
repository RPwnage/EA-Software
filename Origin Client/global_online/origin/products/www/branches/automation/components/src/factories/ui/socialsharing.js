/**
 * Factory to be used to build social sharing components
 * @file socialsharing.js
 */
(function() {
    'use strict';

    var POPOUT_WIDTH = 600,
        POPOUT_HEIGHT = 450;

    /**
     */
    function SocialSharingFactory($window, NavigationFactory, UrlHelper, ComponentsConfigHelper) {
        var socialEndpoints = ComponentsConfigHelper.getUrl('social'),
            facebookShareUrl = socialEndpoints.facebookShareUrl,
            twitterShareUrl = socialEndpoints.twitterShareUrl;

        function openShareWindow (sharePageUrl) {
            if (Origin.client.isEmbeddedBrowser()) {
                window.open(sharePageUrl, '', {
                    width: POPOUT_WIDTH,
                    height: POPOUT_HEIGHT
                });
            } else {
                NavigationFactory.externalUrlInPopupWindow(sharePageUrl, POPOUT_WIDTH, POPOUT_HEIGHT);
            }
        }

        /**
         * Opens Facebook share dialog
         *
         * @param {object} config - config include parameters for share
         *                 { title: '',
         *                   picture: '',
         *                   description: '',
         *                   caption: ''
         *                 }
         */
        function openFacebookShareWindow(config) {
            // Config options. Builds urls for interacting with api.
            var shareConfig = _.extend({u: $window.location.href}, config);
            shareConfig.description = config.description;
            var sharePageUrl = UrlHelper.buildParameterizedUrl(facebookShareUrl, shareConfig);
            openShareWindow(sharePageUrl);
        }

        function openTwitterShareWindow(shareText) {
            // Config options. Builds urls for interacting with twitter api.
            // @todo Get share data from meta data 'og' tags instead of $window.location
            var locale = Origin.locale.locale();
            var sharePageUrl = UrlHelper.buildParameterizedUrl(twitterShareUrl, {
                url: $window.location.href,
                via: 'OriginInsider',
                hashtags: 'OriginStore',
                text: shareText,
                lang: locale
            });

            openShareWindow(sharePageUrl);
        }

        return {
            openFacebookShareWindow: openFacebookShareWindow,
            openTwitterShareWindow: openTwitterShareWindow
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function SocialSharingFactorySingleton($window, NavigationFactory, UrlHelper, ComponentsConfigHelper, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('SocialSharingFactory', SocialSharingFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.SocialSharingFactory
     * @description
     *
     * SocialSharingFactory
     */
    angular.module('origin-components')
        .factory('SocialSharingFactory', SocialSharingFactorySingleton);
}());
