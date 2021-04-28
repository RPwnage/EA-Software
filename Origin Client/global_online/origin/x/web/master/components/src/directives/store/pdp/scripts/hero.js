/**
 * @file store/pdp/hero.js
 */

(function(){
    'use strict';
    /**
     * KeybeatIconTypeEnumeration list of keybeat icons
     * @enum {string}
     * @todo update icon list with appropriate icons
     */
    /* jshint ignore:start */
    var KeybeatIconTypeEnumeration = {
        "No Icon": "",
        "General Origin Keybeat": "originlogo",
        "New Release": "newrelease",
        "DLC": "dlc",
        "Trial": "playwithcircle",
        "Video Available": "video",
        "Pre-order Available": "preorder",
        "Early Access": "earlyaccess",
        "Achievement": "trophy",
        "Save": "store",
        "Expiring": "expiring",
        "Expired": "expired"
    };
    /* jshint ignore:end */

    function originStorePdpHero(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                packArt: '@',
                shortDescription: '@',
                longDescription: '@',
                backgroundImage: '@',
                releaseDateText: '@',
                preloadDateText: '@',
                nowAvailableText: '@',
                readMoreText: '@',
                availableWithSubsText: '@',
                subsInfoText: '@',
                learnMoreText: '@',
                keybeatMessage: '@',
                keybeatIcon: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/hero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpHero
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offer-id product offer ID
     * @param {url|OCD} (optional) pack-art pack art override
     * @param {LocalizedString|OCD} short-description (optional) product subtitle override
     * @param {LocalizedString|OCD} long-description (optional) product description teaser override
     * @param {url|OCD} background-image (optional) background image URL
     * @param {LocalizedString|OCD} release-date-text (optional) release date info string override
     * @param {LocalizedString|OCD} preload-date-text (optional) preload date info string override
     * @param {LocalizedString|OCD} now-available-text (optional) preload availability message override
     * @param {LocalizedString|OCD} read-more-text (optional) read more text override
     * @param {KeybeatIconTypeEnumeration} keybeat-icon icon to use // should be enum
     * @param {LocalizedString} keybeat-message text message
     * @description
     *
     * PDP hero (header container)
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-hero offer-id="{{ offerId }}" background-image="http://docs.x.origin.com/origin-x-design-prototype/images/keyart/battlefront.jpg"></origin-store-pdp-hero>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpHero', originStorePdpHero);
}());
