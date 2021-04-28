
/**
 * @file store/carousel/scripts/product.js
 */
(function(){
    'use strict';



    function OriginStoreCarouselProductCtrl($scope, NavigationFactory){

        $scope.linkFunctionReady = false;
        var stopWatchingLinkFunctionReady;
        $scope.itemCount = 0;
        function onReady(newValue) {
            if (newValue) {
                if (stopWatchingLinkFunctionReady) {
                    stopWatchingLinkFunctionReady();
                }
                $scope.setWrapperVisibility();
            }
        }

        this.registerItem = function(){
            $scope.itemCount++;
            if (!$scope.linkFunctionReady) {
                stopWatchingLinkFunctionReady = $scope.$watch('linkFunctionReady', onReady);
            } else {
                $scope.setWrapperVisibility();
            }
        };

        $scope.onClick = function (e) {
            e.preventDefault();
            NavigationFactory.openLink($scope.viewAllHref);
        };

        $scope.viewAllHrefAbsoluteUrl = NavigationFactory.getAbsoluteUrl($scope.viewAllHref);
    }

    function originStoreCarouselProduct(ComponentsConfigFactory, AppCommFactory) {

        function originStoreCarouselProductLink(scope, element, attrs, originStorePdpSectionWrapperCtrl) {
            scope.setWrapperVisibility = function(){
                if (originStorePdpSectionWrapperCtrl && _.isFunction(originStorePdpSectionWrapperCtrl.setVisibility)){
                    originStorePdpSectionWrapperCtrl.setVisibility(true);
                }
            };

            scope.$watch('itemCount', _.debounce(function(){
                AppCommFactory.events.fire('carousel:resetup');
            }, 300));
            scope.linkFunctionReady = true;
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            require: '?^originStorePdpSectionWrapper',
            scope: {
                titleStr: '@',
                viewAllStr: '@',
                viewAllHref: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/product.html'),
            link: originStoreCarouselProductLink,
            controller: OriginStoreCarouselProductCtrl
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProduct
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title-str - Title of the product carousel
     * @param {LocalizedString} view-all-str - Text for the link that directs a user to another page (ex. solr search or browse page)
     * @param {string} view-all-href - Url for the link that directs a user to another page (ex. solr search or browse page)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-product />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProduct', originStoreCarouselProduct);
}());
