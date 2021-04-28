/**
 * @file home/discovery/tile/config/ProgrammableMerch.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    //These values are used by discovery story factory
    var PlacementTypeEnumeration = {
        "priority": "priority",
        "position": "position"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    
    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    var CalloutTypeEnumeration = {
        "custom": "custom",
        "dynamicPrice": "dynamicPrice",
        "dynamicDiscountPrice": "dynamicDiscountPrice",
        "dynamicDiscountRate": "dynamicDiscountRate"
    };

    /* jshint ignore:end */


    function OriginHomeDiscoveryTileConfigProgrammableMerchCtrl($scope, $attrs, $controller) {

        var tileConfig = {
                directiveName: 'origin-home-discovery-tile-programmable-merch',
                feedData: [{
                    image: $attrs.image,
                    descriptiontitle: $attrs.descriptiontitle,
                    description: $attrs.description,
                    showprice : $attrs.showprice,
                    'callout-text': $attrs.calloutText,
                    'callout-type': $attrs.calloutType,
                    'ocd-path': $attrs.ocdPath,
                    'focused-copy-enabled': $attrs.focusedCopyEnabled,
                    'focused-copy-text-color': $attrs.focusedCopyTextColor
                }],
                limit: 1,
                diminish: 0
        };

        if(!angular.isDefined($attrs.placementtype) || !angular.isDefined($attrs.placementvalue)) {
            tileConfig.position = 1;
        } else {
            tileConfig[$attrs.placementtype] = parseInt($attrs.placementvalue);
        }

        $scope.childDirectives = [];
        tileConfig.childDirectives = $scope.childDirectives;

        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            $scope: $scope,
            tileConfig: tileConfig
        });
    }

    function originHomeDiscoveryTileConfigProgrammableMerch() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigProgrammableMerchCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigProgrammableMerch
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} descriptiontitle the news header
     * @param {LocalizedText} description the description text
     * @param {BooleanEnumeration} showprice show pricing information
     * @param {CalloutTypeEnumeration} callout-type the type of call out, choosing custom will use the value you enter in callout-text
     * @param {LocalizedString} callout-text text that will be replaced into placeholder %callout-text% in headline or text
     * @param {BooleanEnumeration} focused-copy-enabled show the focused version of the copy text
     * @param {TextColorEnumeration} focused-copy-text-color choose the light/dark version of the focused copy text
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @description
     *
     * tile config for programmable merch tile
     *
     */
    //TODO: make this directive standard
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigProgrammableMerchCtrl', OriginHomeDiscoveryTileConfigProgrammableMerchCtrl)
        .directive('originHomeDiscoveryTileConfigProgrammableMerch', originHomeDiscoveryTileConfigProgrammableMerch);

}());