
/**
 * @file store/carousel/hero/scripts/controls.js
 */
(function(){
    'use strict';
    /**
    * The controller
    */
    function OriginStoreCarouselHeroControlsCtrl($scope) {

        $scope.goTo = function(slideNumber){
            $scope.$emit('goTo', {'slideNumber': slideNumber});
        };

        $scope.$on('hideControls', function(){
            $scope.show = false;
        });

        $scope.$on('showControls', function(){
            $scope.show = true;
        });

        $scope.show = true;
    }
    /**
    * The directive
    */
    function originStoreCarouselHeroControls(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                carouselState: '=',
                current : '='
            },
            controller: 'OriginStoreCarouselHeroControlsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/hero/views/controls.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselHeroControls
     * @restrict E
     * @element ANY
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-hero-controls />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreCarouselHeroControlsCtrl', OriginStoreCarouselHeroControlsCtrl)
        .directive('originStoreCarouselHeroControls', originStoreCarouselHeroControls);
}());
