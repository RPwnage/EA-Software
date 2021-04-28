/**
 * @file store/pdp/feature/premieraccolade.js
 */
(function () {
    'use strict';
    /* jshint ignore:start */

    /**
     * list text alignment types
     * @readonly
     * @enum {string}
     */
    var TextAlignmentEnumeration = {
        "left": "left",
        "right": "right"
    };

    /**
     * A list of text colors
     * @readonly
     * @enum {string}
     */
    var TextColorEnumeration = {
        "dark": "dark",
        "light": "light"
    };
    /* jshint ignore:end */

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-feature-premier-wrapper';
    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-premier-accolade';

    function originStorePdpFeaturePremierAccolade(ComponentsConfigFactory, DirectiveScope) {

        function originStorePdpFeaturePremierAccoladeLink(scope, element, attrs, originStorePdpSectionWrapperCtrl) {
            scope.anyDataFound = false;

            function init() {
                if (scope.quote1Str) {
                    scope.anyDataFound = true;

                    if (originStorePdpSectionWrapperCtrl && _.isFunction(originStorePdpSectionWrapperCtrl.setVisibility)){
                        originStorePdpSectionWrapperCtrl.setVisibility(true);
                    }

                    scope.$digest();
                }
            }

            DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE]).then(init);
        }

        return {
            require: '^?originStorePdpSectionWrapper',
            restrict: 'E',
            scope: {
                textColor: '@',
                textAlignment: '@',
                backgroundImage: '@',
                quote1Str: '@',
                quote1SourceStr: '@',
                quote2Str: '@',
                quote2SourceStr: '@',
                quote3Str: '@',
                quote3SourceStr: '@'
            },
            link: originStorePdpFeaturePremierAccoladeLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/premieraccolade.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeaturePremierAccolade
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {TextColorEnumeration} text-color Color of text, light or dark. Note: for left or right alignment & text-color dark, at lower resolutions 1) text-color switches to light and 2) a forgraound color will be applied on top of the background-image
     * @param {TextAlignmentEnumeration} text-alignment Text Alignment, left or right.
     * @param {ImageUrl} background-image The background image for this module.
     * @param {LocalizedString} quote1-str Promo quote 1 - should be wrapped in quotes - use \&#8220; text \&#8221;
     * @param {LocalizedString} quote1-source-str Promo quote 1 source
     * @param {LocalizedString} quote2-str Promo quote 2 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote2-source-str Promo quote 2 source - not visible on small screens
     * @param {LocalizedString} quote3-str Promo quote 3 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote3-source-str Promo quote 3 source - not visible on small screens
     *
     * Large feature image PDP section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-premier-accolade
     *             text-color="light"
     *             text-alignment="left">
     *         </origin-store-pdp-feature-premier-accolade>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeaturePremierAccolade', originStorePdpFeaturePremierAccolade);
}());
