/**
 * @file home/discovery/tile/config/cta.js
 */

(function() {
    'use strict';

    /* jshint ignore:start */
    /**
     * Action ID map used in child directive
     * @readonly
     * @enum {string}
     */
    var ActionIdEnumeration = {
        "url-internal": "url-internal",
        "url-external": "url-external",
        "play-video": "play-video",
        "purchase": "purchase",
        "direct-acquistion": "direct-acquistion",
        "pdp": "pdp",
        "download-install-play": "download-install-play"
    };

    /**
     * A list of button type options used in child directive
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary"
    };
    /* jshint ignore:end */

    function originHomeDiscoveryTileConfigCta() {
        return {
            restrict: 'E',
            scope: {
                actionid: '@',
                href: '@',
                description: '@',
                type: '@'
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {ActionIdEnumeration} actionid The type of cta button
     * @param {Url} href The href associated with this cta
     * @param {LocalizedString} description The button text
     * @param {ButtonTypeEnumeration} type The style of the button 'primary' or 'secondary'
     * @description
     *
     * tile config for owned game recently played discovery tile
     *
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileConfigCta', originHomeDiscoveryTileConfigCta);
}());
