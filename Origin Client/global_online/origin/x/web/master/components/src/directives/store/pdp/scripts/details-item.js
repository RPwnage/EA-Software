/**
 * @file store/pdp/scripts/details-item.js
 */
(function(){
    'use strict';

    /**
     * DetailsItemIconTextEnumeration list of icons to translations
     * @enum {string}
     */
    /* jshint ignore:start */
    var DetailsItemIconTextEnumeration = {
        "otkicon-person": "person",
        "otkicon-multiplayer": "multiplayer",
        "otkicon-coop": "coop",
        "otkicon-controller": "controller",
        "otkicon-trophy": "trophy",
        "otkicon-cloud": "cloud",
        "otkicon-ggg": "ggg",
        "otkicon-download": "download",
        "otkicon-progressiveinstall": "progressiveinstall",
        "otkicon-chatbubble": "chatbubble",
        "otkicon-twitch": "twitch",
        "otkicon-timer": "timer",
        "otkicon-offlinemode": "offlinemode",
        "otkicon-modding": "modding",
        "otkicon-leaderboard": "leaderboard",
        "otkicon-app": "app",
        "otkicon-splitscreen": "splitscreen",
        "otkicon-dolby": "dolby",
        "otkicon-touch": "touch",
        "otkicon-crossplatform" : "crossplatform",
        "otkicon-4k": "4k",
        "otkicon-directx": "directx",
        "otkicon-refresh": "refresh",
        "otkicon-firstperson": "firstperson",
        "otkicon-thirdperson": "thirdperson",
        "otkicon-challenge": "challenge"
    };
    /**
     * DetailItemIconEnumeration list of icons
     * @enum {string}
     */
    var DetailItemIconEnumeration = {
        "otkicon-person": "otkicon-person",
        "otkicon-multiplayer": "otkicon-multiplayer",
        "otkicon-coop": "otkicon-coop",
        "otkicon-controller": "otkicon-controller",
        "otkicon-trophy": "otkicon-trophy",
        "otkicon-cloud": "otkicon-cloud",
        "otkicon-ggg": "otkicon-ggg",
        "otkicon-download": "otkicon-download",
        "otkicon-progressiveinstall": "otkicon-progressiveinstall",
        "otkicon-chatbubble": "otkicon-chatbubble",
        "otkicon-twitch": "otkicon-twitch",
        "otkicon-timer": "otkicon-timer",
        "otkicon-offlinemode": "otkicon-offlinemode",
        "otkicon-modding": "otkicon-modding",
        "otkicon-leaderboard": "otkicon-leaderboard",
        "otkicon-app": "otkicon-app",
        "otkicon-splitscreen": "otkicon-splitscreen",
        "otkicon-dolby": "otkicon-dolby",
        "otkicon-touch": "otkicon-touch",
        "otkicon-crossplatform": "otkicon-crossplatform",
        "otkicon-4k": "otkicon-4k",
        "otkicon-directx": "otkicon-directx",
        "otkicon-refresh": "otkicon-refresh",
        "otkicon-firstperson": "otkicon-firstperson",
        "otkicon-thirdperson": "otkicon-thirdperson",
        "otkicon-challenge": "otkicon-challenge"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-pdp-detailitem';

    function originStorePdpDetailsItem(ComponentsConfigFactory, UtilFactory) {

        function originStorePdpDetailsItemLink(scope) {
            scope.translatedText = UtilFactory.getLocalizedStr(scope.overrideText, CONTEXT_NAMESPACE, scope.key);
        }

        return {
            restrict: 'E',
            scope: {
                icon: '@',
                key: '@',
                overrideText: '@',
                url: '@'
            },
            link: originStorePdpDetailsItemLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/details-item.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDetailsItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {DetailItemIconEnumeration} icon // should be enum
     * @param {DetailsItemIconTextEnumeration} key to use // should be enum
     * @param {LocalizedString} override-text to use
     * @param {Url} url optional URL if a link
     * @description PDP details item block, retrieved from cq5
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-details>
     *         <origin-store-pdp-details-item icon="otkicon-person" text="Single Player" url="http://www.single-player.com">
     *     </origin-store-pdp-details>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpDetailsItem', originStorePdpDetailsItem);
}());
