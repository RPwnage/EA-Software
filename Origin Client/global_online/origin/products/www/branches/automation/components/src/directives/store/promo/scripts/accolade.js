/**
 * @file store/promo/scripts/accolade.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */

    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    /* jshint ignore:end */

    var TextAlignmentEnumeration = {
        "left": "left",
        "right": "right",
        "center": "center"
    };

    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var LayoutMap = {
        'hero': 'l-origin-store-column-hero l-origin-section-divider',
        'full-width': 'l-origin-store-column-full',
        'half-width': 'l-origin-store-column-half'
    };

    var CONTEXT_NAMESPACE = 'origin-store-promo-accolade';

    function originStorePromoAccolade($timeout, ComponentsConfigFactory, UtilFactory, OriginStorePromoConstant) {

        function originStorePromoAccoladeLink(scope, element) {
            scope.layout = scope.layout || LayoutEnumeration.hero;


            element.addClass(LayoutMap[scope.layout]).addClass(OriginStorePromoConstant.layoutClass);


            /**
             * For full-width/half-width variations, override text alignment to be always center.
             */
            function overrideTextAlignment() {
                if (scope.layout !== LayoutEnumeration.hero) {
                    scope.textAlignment = TextAlignmentEnumeration.center;
                } else {
                    scope.textAlignment = scope.textAlignment || TextAlignmentEnumeration.center;
                }
            }


            // Need to wait view to update before overriding textAlignment.
            $timeout(overrideTextAlignment, 0, false);


            scope.numberOfQuotes = _.compact([scope.quote1Str, scope.quote2Str, scope.quote3Str]).length;

            scope.strings = {
                quote1Str: UtilFactory.getLocalizedStr(scope.quote1Str, CONTEXT_NAMESPACE, 'quote1-str'),
                quote2Str: UtilFactory.getLocalizedStr(scope.quote2Str, CONTEXT_NAMESPACE, 'quote2-str'),
                quote3Str: UtilFactory.getLocalizedStr(scope.quote3Str, CONTEXT_NAMESPACE, 'quote3-str'),
                quote1SourceStr: UtilFactory.getLocalizedStr(scope.quote1SourceStr, CONTEXT_NAMESPACE, 'quote1-source-str'),
                quote2SourceStr: UtilFactory.getLocalizedStr(scope.quote2SourceStr, CONTEXT_NAMESPACE, 'quote2-source-str'),
                quote3SourceStr: UtilFactory.getLocalizedStr(scope.quote3SourceStr, CONTEXT_NAMESPACE, 'quote3-source-str')
            };
        }

        return {
            restrict: 'E',
            scope: {
                layout: '@',
                href: '@',
                textColor: '@',
                textAlignment: '@',
                backgroundColor: '@',
                backgroundImage: '@',
                lightGameLogo: '@',
                darkGameLogo: '@',
                quote1Str: '@',
                quote1SourceStr: '@',
                quote2Str: '@',
                quote2SourceStr: '@',
                quote3Str: '@',
                quote3SourceStr: '@'
            },
            link: originStorePromoAccoladeLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/accolade.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoAccolade
     * @restrict E
     * @element ANY
     * @scope
     * @description Mercahndizes a promo module with up to 3 quotes as its content
     * @param {string} background-color hex background color
     * @param {Url} href Url to link to
     * @param {LayoutEnumeration} layout The type of layout this promo will have. Note: for full-width/half-width layout, text-alignment can only be set to center.
     * @param {TextColorEnumeration} text-color Color of text, light or dark. Note: for left or right alignment & text-color dark, at lower resolutions 1) text-color switches to light and 2) a forgraound color will be applied on top of the background-image and 3) dark-game-logo is swapped for light-game-logo
     * @param {TextAlignmentEnumeration} text-alignment Text Alignment, left, right or center. Note: for full-width/half-width layout, text-alignment can only be set to center.
     * @param {ImageUrl} background-image The background image for this module.
     * @param {ImageUrl} light-game-logo Game logo image for this module, used if text-color is light.
     * @param {ImageUrl} dark-game-logo Game logo image for this module, used if text-color is dark.
     * @param {LocalizedString} quote1-str Promo quote 1 - should be wrapped in quotes - use \&#8220; text \&#8221;
     * @param {LocalizedString} quote1-source-str Promo quote 1 source
     * @param {LocalizedString} quote2-str Promo quote 2 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote2-source-str Promo quote 2 source - not visible on small screens
     * @param {LocalizedString} quote3-str Promo quote 3 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote3-source-str Promo quote 3 source - not visible on small screens
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-promo-accolade>
     *
     *          </origin-store-promo-accolade>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originStorePromoAccolade', originStorePromoAccolade);
}());
