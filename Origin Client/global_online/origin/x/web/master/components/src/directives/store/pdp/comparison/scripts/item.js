/**
 * @file store/pdp/comparison/scripts/item.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-comparison-item';

    function OriginStorePdpComparisonItemCtrl($scope, UtilFactory, AppCommFactory) {
        $scope.show = false;
        $scope.showMore = UtilFactory.getLocalizedStr($scope.showMore, CONTEXT_NAMESPACE, 'showMore');
        $scope.showLess = UtilFactory.getLocalizedStr($scope.showLess, CONTEXT_NAMESPACE, 'showLess');

        $scope.showContent = function() {
            $scope.show = true;
        };

        $scope.hideContent = function() {
            $scope.show = false;
        };

        function updateModels(editions) {
            $scope.models = editions;
        }

        AppCommFactory.events.on('storeComparisonTable:updateModels', updateModels);
    }

    function originStorePdpComparisonItem(ComponentsConfigFactory) {

        function originStorePdpComparisonItemLink(scope, ele, attr, ctrl) {
            scope.offerLength = ctrl.getOfferCount();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                title: '@',
                description: '@',
                more: '@',
                image: '@',
                checks: '=',
                showMore: '@',
                showLess: '@'
            },
            controller: OriginStorePdpComparisonItemCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/comparison/views/item.html'),
            require: '^originStorePdpComparisonTable',
            link: originStorePdpComparisonItemLink
        };
    }

    /* jshint ignore:start */
    /**
     * Selects the check marks row
     * @readonly
     * @enum {string}
     */
    var CheksEnumeration = {
        "Checked": "checked",
        "Not Checked": "not-checked",
        "Custom": "custom"
    };
    /* jshint ignore:end */

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpComparisonItem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The title for this table item
     * @param {LocalizedString} description The description paragraph for this table item
     * @param {LocalizedString} more The overflow text to be displayed in the hidden part of the table item
     * @param {ImageUrl} image The small image for this table item
     * @param {CheksEnumeration} checks Select to display a check, x-mark or custom text
     * @param {LocalizedString} text The custom text inserted instead of checks or x-marks
     * @param {LocalizedString} show-more The "Show More" button text
     * @param {LocalizedString} show-less The "Show Less" button text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-comparison-item
     *          title="Battlefield 4 Premium"
     *          description="Get the edge on the battlefield. Premium membership awards a huge assortment of benefits."
     *          more="<br>Benefits include:<br><br><strong>All 5 Expansions:</strong> Get China Rising, Second Assault, Naval Strike, Dragon's Teeth, and Final Stand expansion packs.<br><br><strong>Priority Position:</strong> Gants priority position in the server queue, letting you get in the fight more quickly.<br><br><strong>Personalization Options:</strong> Appearance should be as unique as play style. Express your identity on the battlefield with Premium-exclusive camos, paints, emblems and dogtags.<br><br><strong>Weekly Content:</strong> Level up faster with Premium-exclusive Double XP events and get more out of Battlefield 4 with weekly updates of new content."
     *          image="http://docs.x.origin.com/origin-x-design-prototype/images/compare/bf4_premium.png"
     *          checks="[{type: 'checked'}, {type: 'not-checked'}, {type: 'custom', text: '9 Battlepacks'}]">
     *          show-more="Show More"
     *          show-less="Show Less">
     *      </origin-store-pdp-comparison-item>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpComparisonItemCtrl', OriginStorePdpComparisonItemCtrl)
        .directive('originStorePdpComparisonItem', originStorePdpComparisonItem);
}());