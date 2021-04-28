/**
 * @file store/scripts/backgroundimagecarousel.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */
    var CAROUSEL_ROTATION_TIMER = 6000;

    function OriginStoreBackgroundimagecarouselCtrl($scope){
        $scope.imagesCollection = [].concat($scope.images);
        $scope.currentIndex = 0;

        this.registerBackgroundImage = function(imgSrc){
            $scope.imagesCollection.push(imgSrc);
        };
    }
    function originStoreBackgroundimagecarousel(ComponentsConfigFactory) {
        function originStoreBackgroundimagecarouselLink(scope){
            var interval;
            scope.currentIndex = 0;

            function incrementIndex(){
                scope.currentIndex = ++scope.currentIndex % scope.imagesCollection.length;
                scope.$digest();
            }

            if (scope.imagesCollection.length > 1){
                interval = setInterval(incrementIndex, CAROUSEL_ROTATION_TIMER);
                scope.$on('$destroy', _.partial(clearInterval, interval));
            }
        }
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                backgroundColor: '@',
                images: '=',
                bottomScrim: '@'
            },
            controller: OriginStoreBackgroundimagecarouselCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/backgroundimagecarousel.html'),
            link: originStoreBackgroundimagecarouselLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBackgroundimagecarousel
     * @restrict E
     * @element ANY
     *
     * @param {string} background-color The background color to fade to
     * @param {array} images The array of images to use as the background image
     * @param {BooleanEnumeration} bottom-scrim should this background image have a bottom scrim
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-backgroundimagecarousel background-color='{{ ::backgroundColor }}' image='{{ ::imageSrc }}'> </origin-store-backgroundimagecarousel>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreBackgroundimagecarousel', originStoreBackgroundimagecarousel);
}());
