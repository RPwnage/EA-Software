/**
 * @file game/library/pricing.js
 */
(function() {
    'use strict';

    function originGamePricing(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/pricing.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamePricing
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @description
     *
     * game pricing
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-price offerid="OFB-EAST:109549060"></origin-game-pricing>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originGamePricing', originGamePricing);
}());