/**
 * @file store/scripts/background-mediacarousel.js
 */

(function () {
    'use strict';

    var DEFAULT_INTERVAL_MS = 20000;
    var MAX_FAILURES = 3;

    function originBackgroundMediacarouselCtrl($scope) {
        var callbacks = [];
        var currentIndex = 0;
        var failedTries = 0;
        var stopRotating = false;

        /**
         * Get next active index
         */
        function getNext() {
            var nextIndex = currentIndex + 1;
            if (nextIndex >= callbacks.length) {
                nextIndex = 0;
            }
            return nextIndex;
        }

        /**
         * Notify all carousel items, active index has changed. since all children
         * return a promise, wait for promises to resolve before continuing.
         */
        function notifyAll() {
            if (stopRotating) {
                return;
            }

            var promises = [];
            _.forEach(callbacks, function (callback) {
                promises.push(callback(currentIndex, rotateCarouselItem));
            });


            //wait for all children to resolve their promise.
            if (promises.length > 0) {
                Promise.all(promises).then(rotateCarouselItem);
            } else {
                if (failedTries < MAX_FAILURES) {
                    failedTries++;
                    setTimeout(notifyAll, 200);
                }
            }

        }

        /**
         * increment index and notify all carousel items
         */
        function rotateCarouselItem() {
            currentIndex = getNext();

            notifyAll();
        }

        /**
         * Registers the callback to invoke (for each carousel item) when index changes. Must be a promise.
         * @param {Function} carouselItemCallback callback that returns a promise.
         * @returns {number}
         */
        this.registerCarouselItem = function (carouselItemCallback) {
            callbacks.push(carouselItemCallback);
            return callbacks.length - 1; //return index
        };

        /**
         * get interval between each rotation in milliseconds.
         * @returns {number}
         */
        this.getTimeoutInterval = function () {
            return (parseInt($scope.rotationInterval, 10) * 1000) || DEFAULT_INTERVAL_MS;
        };

        setTimeout(notifyAll, 10);

        $scope.$on('$destroy', function () {
            stopRotating = false;
        });
    }

    function originBackgroundMediacarousel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                rotationInterval: '@'
            },
            transclude: true,
            controller: originBackgroundMediacarouselCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/background-mediacarousel.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originBackgroundMediacarousel
     * @restrict E
     * @element ANY
     * @scope
     * @param {Number} rotation-interval how often images/videos rotate (in seconds). default is 5 seconds.
     * @description
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-background-mediacarousel rotation-interval="5">
     *          <origin-background-mediacarousel-image offer-id="asdasfsa" image-url="asdasdasd"></origin-background-mediacarousel-image>
     *          <origin-background-mediacarousel-video offer-id="asdasfsa" video-id="asdasdasd"></origin-background-mediacarousel-video>
     *     </origin-background-mediacarousel>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originBackgroundMediacarousel', originBackgroundMediacarousel);
}());
