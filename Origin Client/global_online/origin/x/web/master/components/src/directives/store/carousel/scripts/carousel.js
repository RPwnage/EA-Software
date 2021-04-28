
/**
 * @file store/carousel/scripts/carousel.js
 */
(function(){
    'use strict';
    var MAX_RETRIES = 3;
    function decimalFormat(integer){
        return parseInt(integer, 10);
    }

    function OriginStoreCarouselCarouselCtrl($scope, UtilFactory) {
        // Setup the scope
        $scope.carouselState = {};
        $scope.carouselState.current = 0;
        $scope.carouselState.animationRunning = false;
        $scope.isLoading = true;
        $scope.retries = 0;

        /**
         * Animate from one slide to another
         * @param  {int} slideNumber  The virtual slide number, may contain multiple items
         * @return {void}
         */
        $scope.animate = function(slideNumber){

            var distance = slideNumber * $scope.carouselState.maxTilesInView * $scope.carouselState.tileWidth;
            UtilFactory.onTransitionEnd($scope.carouselState.tileContanier, $scope.postSlide, 500);
            $scope.carouselState.animationRunning = true;
            $scope.carouselState.tileContanier.css('transform', 'translate3d(-' + distance + 'px, 0, 0)');
            $scope.carouselState.current = slideNumber;
        };

        /**
         * Manage scope state after a slide animation completes
         * @return {void}
         */
        $scope.postSlide = function() {
            $scope.carouselState.animationRunning = false;
        };

        /**
         * Change to a slide
         * @param  {int} index the id of the slide we wish to move to.
         * @return {void}
         */
        $scope.goTo = function(slideNumber) {

            if(slideNumber === undefined || $scope.carouselState.animationRunning || $scope.carouselState.numberOfSlides.length === 1) {
                return;
            }

            if(slideNumber < 0 ) {
                slideNumber = $scope.carouselState.numberOfSlides.length - 1;
            }

            if(slideNumber >= $scope.carouselState.numberOfSlides.length) {
                slideNumber = 0;
            }

            $scope.animate(slideNumber);
        };

        this.goToNext = function() {
            $scope.goTo($scope.carouselState.current + 1);
        };

        this.goToPrev = function() {
            $scope.goTo($scope.carouselState.current - 1);
        };

    }

    function originStoreCarouselCarousel(ComponentsConfigFactory, AppCommFactory, UtilFactory, $compile, $timeout) {

        function originStoreCarouselCarouselLink(scope, elem, attrs, ctrl) {
            /**
             * This function runs to create the carousel. This function needs to be only run once at the initial load of the
             * carousel
             *
             * @return {void}
             */
            function initialize(){
                // Add on the indicators and the controls
                if(scope.version !== 'MODAL'){
                    var indicator = $compile('<origin-store-carousel-product-indicators carousel-state="carouselState"></origin-store-carousel-product-indicators>')(scope);
                    elem.append(indicator);
                }

                var controls = $compile('<origin-store-carousel-product-controls carousel-state="carouselState"></origin-store-carousel-product-controls>')(scope);
                elem.find('.origin-store-carousel-carousel-header').append(controls);

                // Setup Events
                scope.$on('goTo', function(event, data){
                    event.stopPropagation();
                    scope.goTo(data.slideNumber);
                });

                elem.on('carousel:setup DOMSubtreeModified propertychange', function (event) {
                    event.stopPropagation();
                    setup();
                });

                elem.on('carousel:finished', function (event) {
                    event.stopPropagation();
                    finishedLoading();
                });

                AppCommFactory.events.on('carousel:resetup', resetCarousel);

                // bind to swipe events only on touch-enabled devices
                if (UtilFactory.isTouchEnabled()) {
                    elem.on('swipeleft', ctrl.goToNext);
                    elem.on('swiperight', ctrl.goToPrev);
                }

                setup();
            }

            function resetCarousel() {
                setup();
                scope.animate(scope.carouselState.current);
            }

            /**
             * A function to setup the carousel after major changes have been made to it's state.  This function can be run multiple
             * times to set the carousel in the correct state after major events ie number of slides change after a timer expires.
             *
             * @return {void}
             */
            function setup() {
                var carousel = elem.find('.origin-store-carousel-carousel-header'),
                    tileContanier = elem.find('.origin-store-carousel-carousel-slidearea'),
                    productTiles = tileContanier.children(),
                    first = productTiles.first(),
                    tileWidth = first.outerWidth() + decimalFormat(first.css('margin-left')) + decimalFormat(first.css('margin-right')),
                    tileContanierWidth = Math.ceil(tileWidth * scope.totalTiles + tileContanier.css('padding-left') + tileContanier.css('padding-right'));

                if(productTiles.length > 0){
                    finishedLoading();
                }

                tileContanier.width(tileContanierWidth + 'px');

                var carouselWidth = carousel.width();

                scope.carouselState.tileWidth = tileWidth;
                scope.carouselState.tileContanier = tileContanier;
                scope.carouselState.totalTiles = productTiles.length;
                scope.carouselState.maxTilesInView = Math.floor(carouselWidth / tileWidth);
                scope.carouselState.overHang = carouselWidth - (scope.carouselState.maxTilesInView *  scope.carouselState.tileWidth);
                scope.carouselState.maxPages = Math.ceil(productTiles.length/Math.floor(carouselWidth / tileWidth)) - 1;

                if(scope.carouselState.totalTiles > 0 && scope.carouselState.maxTilesInView > 0) {
                    scope.carouselState.numberOfSlides = new Array(Math.ceil(scope.carouselState.totalTiles / scope.carouselState.maxTilesInView));
                } else {
                    scope.retries++;
                    if(scope.retries < MAX_RETRIES){
                        $timeout(setup, 200); // This shouldn't really be here. The modal isn't drawing in the correct time
                    }
                    scope.carouselState.numberOfSlides = 0;
                }

                if (scope.carouselState.current > scope.carouselState.maxPages) {
                    scope.animate(scope.carouselState.maxPages);
                }
            }

            /**
             * Set loading state to "false" to hide spinner and indicate to user that nothing is loading anymore - 
             * even in the case of a failed request.
             *
             * @return {void}
             */
            function finishedLoading() {
                $timeout(function() {
                    scope.isLoading = false;
                    scope.$digest();
                }, 200);
            }

            /**
             * Update the UI and rerun the setup when the window size changes.
             *
             * @return {void}
             */
            function resize() {
                $timeout.cancel(scope.resizeTimeout);
                scope.resizeTimeout = $timeout(function() {
                    setup();
                    while(scope.carouselState.current >= scope.carouselState.numberOfSlides.length){
                        scope.carouselState.current--;
                    }
                    scope.animate(scope.carouselState.current);
                }, 250);
            }

            scope.$on('$destroy', function () {
                AppCommFactory.events.off('carousel:resetup', resetCarousel);
                $(window).off('resize');
            });

            $timeout(initialize, 0);
            $(window).resize(resize);
        }

        return {
            restrict: 'A',
            scope: false,
            controller: 'OriginStoreCarouselCarouselCtrl',
            link: originStoreCarouselCarouselLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselCarousel
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-carousel />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreCarouselCarouselCtrl', OriginStoreCarouselCarouselCtrl)
        .directive('originStoreCarouselCarousel', originStoreCarouselCarousel);
}());
