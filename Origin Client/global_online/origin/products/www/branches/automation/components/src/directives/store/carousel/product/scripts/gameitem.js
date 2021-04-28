/**
 * @file store/carousel/product/scripts/gameitem.js
 *
 */
(function () {
    'use strict';

    /**
     * A list of tile type options
     * @readonly
     * @enum {string}
     */
    var TileTypeEnumeration = {
        "fixed": "fixed",
        "responsive": "responsive",
        "compact": "compact"
    };

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-carousel-product-wrapper';
    var CONTEXT_NAMESPACE = 'origin-store-carousel-product-gameitem';

    function originStoreCarouselProductGameitem(ComponentsConfigFactory, DirectiveScope) {

        function originStoreCarouselProductGameitemLink(scope, element, attrs, controllers) {
            var compactedControllers = _.compact(controllers);

            function setType() {
                scope.type = TileTypeEnumeration[scope.type] || TileTypeEnumeration.fixed;
            }

            if (compactedControllers.length === 2) { //need to look up ocd ONLY if gameitem is on pdp (i.e. wrapped in originStorePdpSectionWrapper)
                var originStoreCarouselProductCtrl = controllers[0];
                DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE]).then(function () {
                    if (scope.ocdPath && originStoreCarouselProductCtrl && _.isFunction(originStoreCarouselProductCtrl.registerItem)) {
                        originStoreCarouselProductCtrl.registerItem();
                    }

                    setType();
                });
            } else {
                DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE).then(setType);
            }
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                ocdPath: '@',
                type: '@',
                overrideVault: '@'
            },
            require: ['?^originStoreCarouselProduct', '?^originStorePdpSectionWrapper'],
            link: originStoreCarouselProductGameitemLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/product/views/gameitem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProductGameitem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {OcdPath} ocd-path OCD Path of the product
     * @param {TileTypeEnumeration} type can be one of three values 'fixed'/'responsive'/'compact'. this sets the style of the tile, Default is responsive
     * @param {BooleanEnumeration} override-vault Override the "included with vault" text
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-product-gameitem ocd-path=""/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProductGameitem', originStoreCarouselProductGameitem);
}());
