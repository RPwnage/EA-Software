/**
 * @file home/discovery/tile/config/programmable.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
   //These values are used by discovery story factory
    var PlacementTypeEnumeration = {
        "priority": "priority",
        "placement": "placement"
    };
    /* jshint ignore:end */

    function originHomeDiscoveryTileConfigProgrammable(DiscoveryStoryFactory) {

        function originHomeDiscoveryTileConfigProgrammableLink(scope, element, attrs, parentCtrl) {
            var merchandisedValues = {},
                ctaButtons = [],
                a,
                i;

            for (a in attrs.$attr) {
                if (attrs.hasOwnProperty(a)) {
                    merchandisedValues[a] = attrs[a];
                }
            }
            ctaButtons = element.children('origin-home-discovery-tile-config-cta');
            merchandisedValues.cta = [];
            for (i = 0; i < ctaButtons.length; i++) {
                merchandisedValues.cta[i] = {};
                merchandisedValues.cta[i].actionid = $(ctaButtons[i]).attr('actionid');
                merchandisedValues.cta[i].href = $(ctaButtons[i]).attr('href');
                merchandisedValues.cta[i].description = $(ctaButtons[i]).attr('description');
                merchandisedValues.cta[i].type = $(ctaButtons[i]).attr('type');
            }
            DiscoveryStoryFactory.updateProgramStoryTypeConfig(parentCtrl.bucketId(), merchandisedValues);
        }

        return {
            restrict: 'E',
            require: '^originHomeDiscoveryBucket',
            link: originHomeDiscoveryTileConfigProgrammableLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigProgrammable
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image the image
     * @param {LocalizedString} description the description text
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @description
     *
     * tile config for programmable tile discovery tile
     *
     */
    //TODO: make this directive standard
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileConfigProgrammable', originHomeDiscoveryTileConfigProgrammable);

}());