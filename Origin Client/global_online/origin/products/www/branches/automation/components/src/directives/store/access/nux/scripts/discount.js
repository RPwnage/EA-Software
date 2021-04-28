
/**
 * @file store/access/nux/scripts/discount.js
 */
(function(){
    'use strict';

    function OriginStoreAccessNuxDiscountCtrl($scope, $element, DialogFactory, LocalStorageFactory, CheckoutFactory, ComponentsConfigHelper, storeAccessNux) {
        var count = (_.size($element.parent().children()) - 1);

        $scope.currentIndex = $element.index();

        $scope.getPillCount = function() {
            return new Array(count);
        };

        $scope.nextStage = function() {
            if($element.index() !== count) {
                $element
                    .removeClass(storeAccessNux.class)
                    .next()
                    .addClass(storeAccessNux.class);
                LocalStorageFactory.set(storeAccessNux.key, $element.index()+1);
            } else {
                DialogFactory.close(storeAccessNux.id);
                LocalStorageFactory.delete(storeAccessNux.key);
                if (ComponentsConfigHelper.isOIGContext()) {
                    CheckoutFactory.close();
                }
            }
        };

        if(LocalStorageFactory.get(storeAccessNux.key) === $element.index()) {
            $element.addClass(storeAccessNux.class);
        }
    }
    function originStoreAccessNuxDiscount(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                btn: '@',
                description: '@',
                image: '@'
            },
            controller: 'OriginStoreAccessNuxDiscountCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/nux/views/discount.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessNuxDiscount
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title-str The title for this module
     * @param {LocalizedString} btn The button text for this module
     * @param {LocalizedString} description The description for this module
     * @param {ImageUrl} image The background image for this module.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-nux-discount
     *          title-str="blah"
     *          btn="Lets Go"
     *          description="some text"
     *          image="http://someimage.jpg">
     *     </origin-store-access-nux-discount>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessNuxDiscountCtrl', OriginStoreAccessNuxDiscountCtrl)
        .directive('originStoreAccessNuxDiscount', originStoreAccessNuxDiscount);
}());
