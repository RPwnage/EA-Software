/**
 * @file store/promo/scripts/badge.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */

    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    /* jshint ignore:end */

    var LayoutEnumeration = {
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var LayoutMap = {
        'hero': 'l-origin-store-column-hero l-origin-section-divider',
        'full-width': 'l-origin-store-column-full',
        'half-width': 'l-origin-store-column-half'
    };

    var CONTEXT_NAMESPACE = 'origin-store-promo-badge';

    function originStorePromoBadge(ComponentsConfigFactory, UtilFactory, OriginStorePromoConstant) {

        function originStorePromoBadgeLink(scope, element) {
            scope.layout = scope.layout || LayoutEnumeration['full-width'];

            element.addClass(LayoutMap[scope.layout]).addClass(OriginStorePromoConstant.layoutClass);

            scope.strings = {
                quoteStr: UtilFactory.getLocalizedStr(scope.quoteStr, CONTEXT_NAMESPACE, 'quote-str')
            };
        }

        return {
            restrict: 'E',
            scope: {
                layout: '@',
                href: '@',
                textColor: '@',
                backgroundColor: '@',
                backgroundImage: '@',
                badgeImage: '@',
                quoteStr: '@'
            },
            link: originStorePromoBadgeLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/badge.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoBadge
     * @restrict E
     * @element ANY
     * @scope
     * @description Mercahndizes a promo module with up to 3 quotes as its content
     * @param {string} background-color hex background color
     * @param {Url} href Url to link to
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {TextColorEnumeration} text-color Color of text, light or dark.
     * @param {ImageUrl} background-image The background image for this module.
     * @param {ImageUrl} badge-image The badge image for this module.
     * @param {LocalizedString} quote-str Promo quote
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-promo-badge>
     *
     *          </origin-store-promo-badge>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originStorePromoBadge', originStorePromoBadge);
}());
