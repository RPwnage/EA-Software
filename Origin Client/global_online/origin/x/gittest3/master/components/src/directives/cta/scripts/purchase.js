/**
 * @file cta/purchase.js
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

    function OriginCtaPurchaseCtrl($scope, DialogFactory, ComponentsLogFactory) {
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function() {
            //load External url here with $scope.link
            ComponentsLogFactory.log('CTA: Purchasing something');
            DialogFactory.openAlert({
                id: 'web-purchasing-something',
                title: 'Buying Something',
                description: 'When the rest of OriginX is fully functional, you will be taken to the checkout page with the item in your cart.',
                rejectText: 'OK'
            });
        };

    }

    function originCtaPurchase(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                link: '@href',
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaPurchaseCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaPurchase
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {Url} href the purchase link
     * @description
     *
     * Purchase (takes you to shopping cart) call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-purchase href="http://somepurchaselink" description="Buy me" type="primary"></origin-cta-purchase>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaPurchaseCtrl', OriginCtaPurchaseCtrl)
        .directive('originCtaPurchase',  originCtaPurchase);
}());