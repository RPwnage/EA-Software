/**
 * @file home/merchandise/promocode.js
 */
(function() {
    'use strict';

    /**
     * Home Merchandise Tile Callout directive
     * @directive originHomeMerchandiseCalloutTile
     */
    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };
    var CONTEXT_NAMESPACE = 'origin-home-merchandise-promocode';
    function originHomeMerchandisePromocode(ComponentsConfigFactory, UtilFactory) {
        function originHomeMerchandisePromocodeLink(scope){
            scope.textColor = TextColorEnumeration[scope.textColor] || TextColorEnumeration.dark;
            scope.textColorClass = 'origin-home-merchandise-text-color-' + scope.textColor;

            function buildPromocodeHtml(promoCode){
                var html='';
                if (promoCode){
                    html = '<span class="origin-home-merchtile-promocode-promocode">' + promoCode + '</span>';
                }
                return html;

            }

            scope.textFormatted = UtilFactory.getLocalizedStr(scope.text, CONTEXT_NAMESPACE, 'text', {
                '%promo-code%': buildPromocodeHtml(scope.promoCode)
            });

        }
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                ctid: '@',
                imagepath: '@',
                headline: '@',
                subheadline: '@',
                text: '@',
                promoCode: '@',
                legalDisclaimerText: '@',
                legalDisclaimerLinkText: '@',
                legalDisclaimerDialogHeaderText: '@',
                legalDisclaimerDialogBodyTitleText: '@',
                legalDisclaimerDialogBodyText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/merchandise/views/promocode.html'),
            controller: 'OriginHomeMerchandiseTileCtrl',
            link: originHomeMerchandisePromocodeLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeMerchandisePromocode
     * @restrict E
     * @element ANY
     * @scope
     * @param {String} ctid Campaign Targeting ID
     * @param {ImageUrl} imagepath image in background
     * @param {LocalizedString} headline title text
     * @param {LocalizedString} subheadline sub title text
     * @param {LocalizedString} text description text
     * @param {LocalizedString} promo-code promo code, will get replaced into %promo-code% in text
     * @param {TextColorEnumeration} text-color dark or light to match background image. Defaults to dark
     * @param {LocalizedString} legal-disclaimer-text legal claimer text
     * @param {LocalizedString} legal-disclaimer-link-text - legal disclaimer link text. Will be replaced into placeholder %legal-disclaimer-link-text% in legal-disclaimer-text
     * @param {LocalizedString} legal-disclaimer-dialog-header-text legal disclaimer dialog header text.
     * @param {LocalizedString} legal-disclaimer-dialog-body-title-text legal disclaimer dialog body title
     * @param {LocalizedString} legal-disclaimer-dialog-body-text legal disclaimer dialog body text
     *
     * @description
     *
     * merchandise callout tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-merchandise-promocode headline="Go All In. Save 30% On Premium." text="Upgrade to all maps, modes, vehicles and more for one unbeatable price." imagepath="/content/dam/originx/web/app/games/batman-arkham-city/181544/tiles/tile_batman_arkham_long.jpg" alignment="right" theme="dark" class="ng-isolate-scope" ctid="campaing-id-1"></origin-home-merchandise-promocode>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeMerchandisePromocode', originHomeMerchandisePromocode);

}());