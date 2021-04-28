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
        "otkicon-4k": "fourK",
        "otkicon-directx": "directx",
        "otkicon-refresh": "refresh",
        "otkicon-firstperson": "firstperson",
        "otkicon-thirdperson": "thirdperson",
        "otkicon-mouse": "mouse",
        "otkicon-ggg": "ggg",
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
        "otkicon-challenge": "otkicon-challenge",
        "otkicon-mouse": "otkicon-mouse",
        "otkicon-ggg": "otkicon-ggg"
    };
    /* jshint ignore:end */

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-details-wrapper';
    var CONTEXT_NAMESPACE = 'origin-store-pdp-details-item';

    function originStorePdpDetailsItem(ComponentsConfigFactory, UtilFactory, DirectiveScope, NavigationFactory) {

        function originStorePdpDetailsItemLink(scope, element, attributes, originStorePdpDetailsCtrl) {
            DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE]).then(function() {
                if (scope.key) {
                    scope.translatedText = UtilFactory.getLocalizedStr(scope.overrideText, CONTEXT_NAMESPACE, scope.key);
                    originStorePdpDetailsCtrl.registerItemWithData();
                    scope.$digest();
                }
                if (scope.url){
                    scope.openLink = function(url, $event){
                        $event.preventDefault();
                        NavigationFactory.openLink(url);
                    };
                    scope.absoluteUrl = NavigationFactory.getAbsoluteUrl(scope.url);
                }
            });
        }

        return {
            restrict: 'E',
            replace: true,
            require: '^originStorePdpDetails',
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
     * @param {DetailItemIconEnumeration} icon icon to display
     * @param {DetailsItemIconTextEnumeration} key description for icon
     * @param {LocalizedString} override-text text to override default
     * @param {Url} url optional URL if a link
     *
     * @param {LocalizedText} person details item person text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} multiplayer details item multiplayer text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} coop details item coop text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} controller details item controller text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} trophy details item trophy text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} cloud details cloud person text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} ggg details item ggg text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} download details item download text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} progressiveinstall details item progressiveinstall text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} chatbubble details item chatbubble text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} twitch details item twitch text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} timer details item timer text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} offlinemode details item offlinemode text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} modding details item modding text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} leaderboard details item leaderboard text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} app details item app text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} splitscreen details item splitscreen text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} dolby details item dolby text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} touch details item touch text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} crossplatform details item crossplatform text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} fourK details item 4k text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} directx details item directx text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} refresh details item refresh text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} firstperson details item firstperson text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} thirdperson details item thirdperson text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     * @param {LocalizedText} challenge details challenge person text, does not impact component unless the key matching this value is passed. Meant to be set up once in default
     *
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
