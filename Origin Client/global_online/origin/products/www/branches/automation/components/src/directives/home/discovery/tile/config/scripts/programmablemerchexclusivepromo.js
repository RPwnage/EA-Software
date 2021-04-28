/**
 * @file home/discovery/tile/config/programmablemerchxclusivepromo.js
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

    function OriginHomeDiscoveryTileConfigProgrammableMerchExclusivePromoCtrl($scope, $attrs, $controller) {

        var tileConfig = {
            directiveName: 'origin-home-discovery-tile-programmable-merch',
            feedData: [{
                image: $attrs.image,
                descriptiontitle: $attrs.descriptiontitle,
                description: $attrs.description,
                showprice: $attrs.showprice,
                'callout-text': $attrs.calloutText,
                'callout-type': $attrs.calloutType,
                'ocd-path': $attrs.ocdPath,
                'promo-code': $attrs.promoCode,
                'promo-legal-id': $attrs.promoLegalId,
                'promo-btn-text': $attrs.promoBtnText,
                'btn-text': $attrs.btnText,
                'exclusive-promo': true,
                'focused-copy-enabled': $attrs.focusedCopyEnabled,
                'focused-copy-text-color': $attrs.focusedCopyTextColor
            }],
            limit: 1,
            diminish: 0
        };

        if (!angular.isDefined($attrs.placementtype) || !angular.isDefined($attrs.placementvalue)) {
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

    function originHomeDiscoveryTileConfigProgrammableMerchExclusivePromo() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigProgrammableMerchExclusivePromoCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigProgrammableMerchExclusivePromo
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} descriptiontitle the news header
     * @param {LocalizedText} description the description text
     * @param {BooleanEnumeration} showprice show pricing information
     * @param {LocalizedString} callout-text text that will be replaced into placeholder %callout-text% in headline or text if the callout-type is custom
     * @param {CalloutTypeEnumeration} callout-type the type of the callout text - dynamicPrice/dynamicDiscountedPrice/dynamicDiscountedRate will pull from the SPA pricing logic
     * @param {OcdPath} ocd-path ocd path related to tile
     * @param {String} promo-code Promo code to be used
     * @param {String} promo-legal-id Id that is used for the promotion on the terms and conditions page. This will correspond to the 'id' field of the origin-policyitem that is merch'd for this promotion.
     * @param {LocalizedString} promo-btn-text Text for the purchase promo button (if excusivePromo is set to true)
     * @param {BooleanEnumeration} focused-copy-enabled show the focused version of the copy text
     * @param {TextColorEnumeration} focused-copy-text-color choose the light/dark version of the focused copy text
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @description
     *
     * tile config for programmable merch exclusive promo tile
     *
     */

    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigProgrammableMerchExclusivePromoCtrl', OriginHomeDiscoveryTileConfigProgrammableMerchExclusivePromoCtrl)
        .directive('originHomeDiscoveryTileConfigProgrammableMerchExclusivePromo', originHomeDiscoveryTileConfigProgrammableMerchExclusivePromo);

}());