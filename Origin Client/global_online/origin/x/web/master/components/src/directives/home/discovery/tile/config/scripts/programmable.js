/**
 * @file home/discovery/tile/config/programmable.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
   //These values are used by discovery story factory
    var PlacementTypeEnumeration = {
        "priority": "priority",
        "placement": "position"
    };

    var FriendsListEnumeration = {
        "friendsPlayedOrPlaying": "friendsPlayedOrPlaying",
        "friendsOwned": "friendsOwned"
    };

    var ShowPriceEnumeration = {
        "true": "true",
        "false": "false"
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
     * @param {LocalizedString} authorname an optional author name eg. JohnDoe253
     * @param {LocalizedString} authordescription an optional author description eg. Youtube User
     * @param {Url} authorhref an optional author external link
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @param {FriendsListEnumeration} friendslisttype the friends list type you want to show
     * @param {LocalizedText} friendstext the friends text associated with the avatar list
     * @param {LocalizedText} friendspopouttitle the title shown on the friends popout title
     * @param {string} offerid the offer id used for pricing/friends list
     * @param {ShowPriceEnumeration} showprice do we want to show a price if there is a valid offer id associated with the programmable tile
     * @description
     *
     * tile config for programmable tile discovery tile
     *
     */
    //TODO: make this directive standard
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileConfigProgrammable', originHomeDiscoveryTileConfigProgrammable);

}());