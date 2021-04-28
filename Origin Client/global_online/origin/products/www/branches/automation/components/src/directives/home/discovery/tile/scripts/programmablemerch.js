/**
 * @file home/discovery/tile/programmablemerch.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileProgrammableMerch(ComponentsConfigFactory, PriceInsertionFactory, GamesDataFactory, ComponentsLogFactory, PurchaseFactory, InfobubbleFactory, OcdPathFactory, ScopeHelper, UtilFactory, AppCommFactory) {

        var CONTEXT_NAMESPACE = 'origin-home-merchandise-tile',
            INFOBUBBLE_NAMESPACE = 'origin-dialog-content-mobile-infobubble',
            HOVER_CLASS = 'origin-home-exclusive-promo-hover';

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

        function addExclusivePromoFeatures(scope, element) {

            function buy() {
                PurchaseFactory.buy(scope.model.offerId, {promoCode: scope.promoCode});
            }

            scope.openMobileInfobubble = function () {
                var infobubbleParams = {
                    offerId: scope.model.offerId,
                    actionText: scope.btnText,
                    viewDetailsText: scope.viewDetailsText,
                    promoLegalId: scope.promoLegalId
                };

                if (UtilFactory.isTouchEnabled()) {
                    InfobubbleFactory.openDialog(infobubbleParams, buy);
                }
            };

            scope.viewDetailsText = UtilFactory.getLocalizedStr(scope.viewDetailsText, INFOBUBBLE_NAMESPACE, 'view-details-text');

            //promo button uses these button text
            scope.btnText = scope.promoBtnText;

            scope.$watchOnce('model', function () {
                scope.infobubbleContent =  '<origin-store-game-rating></origin-store-game-rating>'+
                    '<origin-store-game-legal></origin-store-game-legal>';
            });

            scope.$watch('model.isOwned', function(isOwned, oldValue){
                if (isOwned && isOwned !== oldValue) {
                    element.remove();
                    AppCommFactory.events.fire('origin-route:reloadcurrentRoute');
                }
            });

            OcdPathFactory.get(scope.ocdPath).attachToScope(scope, 'model');

            /* This adds a hover class when the infobubble is hovered */
            scope.$on('infobubble-show', function() {
                element.addClass(HOVER_CLASS);
            });

            /* this removes the class */
            scope.$on('infobubble-hide', function() {
                element.removeClass(HOVER_CLASS);
            });            
        }

        function addFocusedCopyFeatures(scope) {
            scope.focusedCopyTextColor = TextColorEnumeration[scope.focusedCopyTextColor] || TextColorEnumeration.dark;
            scope.textColorClass = 'origin-home-merchandise-text-color-' + scope.focusedCopyTextColor;                
        }

        function originHomeDiscoverTileProgrammableMerchLink(scope, elem, attr, ctrl, $transclude) {
            $transclude(function(clone) {
                //if there are no elements passed in for transclude it means we have no buttons
                //so don't show the hover
                scope.showHover = clone.length || scope.exclusivePromo;
            });

            scope.priceVisible = scope.showprice === BooleanEnumeration.true;
            scope.strings = {};

            function buildCalloutHtml(calloutText) {
                return calloutText ? '<span class="callout-text">' + calloutText + '</span>' : '';
            }

            function retrievePrice(field, offerId) {
                var useCustomCalloutText = scope.calloutType === CalloutTypeEnumeration.custom,
                    priceSubstitutionToken = '%~' + offerId + '~%',
                    replacementObject = {
                        '%callout-text%': buildCalloutHtml(useCustomCalloutText ? scope.calloutText : priceSubstitutionToken)
                    };

                if ((scope.calloutType === CalloutTypeEnumeration.dynamicPrice) || (scope.calloutType === CalloutTypeEnumeration.custom))  {
                    PriceInsertionFactory.insertPriceIntoLocalizedStr(scope, scope.strings, scope[field], CONTEXT_NAMESPACE, field, replacementObject);
                } else if (scope.calloutType === CalloutTypeEnumeration.dynamicDiscountPrice) {
                    PriceInsertionFactory
                        .insertDiscountedPriceIntoLocalizedStr(scope, scope.strings, scope[field], CONTEXT_NAMESPACE, field, replacementObject);
                } else if (scope.calloutType === CalloutTypeEnumeration.dynamicDiscountRate) {
                    PriceInsertionFactory
                        .insertDiscountedPercentageIntoLocalizedStr(scope, scope.strings, scope[field], CONTEXT_NAMESPACE, field, replacementObject);
                } else {
                    //no replacement needed
                    scope.strings[field] = scope[field];
                }
            }

            function setOfferId(offerId) {
                scope.offerId = offerId;
                retrievePrice('description', offerId);
                retrievePrice('descriptiontitle', offerId);
                ScopeHelper.digestIfDigestNotInProgress(scope);
            }

            function handleError(error) {
                ComponentsLogFactory.error('[origin-home-merchandise-tile]', error);
            }

            GamesDataFactory.lookUpOfferIdIfNeeded(scope.ocdPath)
                .then(setOfferId)
                .catch(handleError);

            if(scope.exclusivePromo) {
                addExclusivePromoFeatures(scope, elem);
            }

            if(scope.focusedCopyEnabled) {
                addFocusedCopyFeatures(scope);
            }
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                image: '@',
                descriptiontitle: '@',
                description: '@',
                showprice: '@',
                calloutText: '@',
                calloutType:'@',
                ocdPath: '@',
                promoCode: '@',
                promoLegalId: '@',
                promoBtnText: '@',
                exclusivePromo: '@',
                focusedCopyEnabled: '@',
                focusedCopyTextColor:'@',
                ctid: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/programmablemerch.html'),
            controller: 'OriginHomeDiscoveryTileProgrammableActionNewsCtrl',
            link: originHomeDiscoverTileProgrammableMerchLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileProgrammableMerch
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} descriptiontitle the news header
     * @param {LocalizedText} description the description text
     * @param {BooleanEnumeration} showprice show pricing information
     * @param {LocalizedString} callout-text text that will be replaced into placeholder %callout-text% in headline or text if the callout-type is custom
     * @param {CalloutTypeEnumeration} callout-type the type of the callout text - dynamicPrice/dynamicDiscountedPrice/dynamicDiscountedRate will pull from the SPA pricing logic
     * @param {OcdPath} ocd-path ocd path related to tile
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {String} promo-code Promo code to be used
     * @param {String} promo-legal-id Id that is used for the promotion on the terms and conditions page. This will correspond to the 'id' field of the origin-policyitem that is merch'd for this promotion.
     * @param {LocalizedString} promo-btn-text Text for the purchase promo button (if excusivePromo is set to true)
     * @param {BooleanEnumeration} exclusive-promo true if this merch tile is the exclusive promo style
     * @param {BooleanEnumeration} focused-copy-enabled show the focused version of the copy text
     * @param {TextColorEnumeration} focused-copy-text-color choose the light/dark version of the focused copy text
     * @description
     *
     * programmable merch tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-home-discovery-tile-config-programmable-news image="https://data1.origin.com/live/content/dam/originx/web/app/programs/MyNewsCommunity/Sep/TF2_Pilot_Trailer_Logo.jpg" descriptiontitle="EXPLORE SIMCITY BUILDIT" description="Welcome, Mayors! Three weeks ago, our Helsinki studio released SimCity BuildIt - an all-new version of SimCity for a new generation of players on smartphones and tablets." placementtype="position" placementvalue="1" authorname="bc" authordescription="description for bc" authorhref="http://www.espn.com" timetoread="4 minutes">
     *               <origin-home-discovery-tile-config-cta actionid="url-external" href="http://www.ea.com/simcity-buildit/" description="Create Your SimCity" type="primary"></origin-home-discovery-tile-config-cta>
     *            </origin-home-discovery-tile-config-programmable-news>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileProgrammableMerch', originHomeDiscoveryTileProgrammableMerch);
}());