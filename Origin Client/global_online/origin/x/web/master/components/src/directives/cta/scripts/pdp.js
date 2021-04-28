/**
 * @file cta/pdp.js
 */

(function() {
    'use strict';

    var CLS_ICON_LEARNMORE = 'otkicon-learn-more';

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };

    function OriginCtaPdpCtrl($scope, PdpUrlFactory) {
        $scope.model = {};
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function($event) {
            $event.stopPropagation();

            PdpUrlFactory.goToPdp({
                offerId: $scope.offerId
            });
        };
    }

    function originCtaPdp(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaPdpCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaPdp
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {offerId} offer-id the ID of the offer
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-pdp offer-id="OFFER-123" description="Take me to PDP" type="primary"></origin-cta-pdp>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaPdpCtrl', OriginCtaPdpCtrl)
        .directive('originCtaPdp', originCtaPdp);
}());