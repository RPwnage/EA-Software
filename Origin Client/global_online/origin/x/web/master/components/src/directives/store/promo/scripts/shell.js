
/** 
 * @file store/promo/scripts/shell.js
 */ 
(function(){
    'use strict';
    function OriginStorePromoShellCtrl($scope, NavigationFactory, ProductFactory) {
        $scope.onBtnClick = function(e, url) {
            var href = url || $scope.href;
            
            if(href){
                NavigationFactory.openLink(href);
            }
            e.stopPropagation();
        };

        if($scope.offerid) {
            ProductFactory.get($scope.offerid).attachToScope($scope, 'product');
        }
    }

    function originStorePromoShell(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            controller: 'OriginStorePromoShellCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/shell.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoShell
     * @restrict E
     * @element ANY
     * @description Contsructs the basic scaffolding for a promo item, including the background image 
     * and a placeholder for the banner. Transcludes a banner. Used to build out a promo item.
     *
     * @example
     *   <origin-store-promo-shell>
     *       <origin-store-promo-banner-static></origin-store-promo-banner-static>
     *   </origin-store-promo-shell>
     */
    angular.module('origin-components')
        .controller('OriginStorePromoShellCtrl', OriginStorePromoShellCtrl)
        .directive('originStorePromoShell', originStorePromoShell);
}()); 
