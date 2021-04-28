/**
 * @file store/scripts/background-mediacarouselimage.js
 */

(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-background-mediacarousel-image';

    function originBackgroundMediacarouselItemCtrl($scope, PdpUrlFactory) {
        $scope.isOnTop = false;
        $scope.goToPdp = function () {
            PdpUrlFactory.goToPdp($scope.model);
        };

        function onDestroy() {
            if ($scope.timerHandle) {
                clearTimeout($scope.timerHandle);
            }
        }

        $scope.$on('$destroy', onDestroy);
    }

    function originBackgroundMediacarouselImage(ComponentsConfigFactory, DirectiveScope) {

        function originBackgroundMediacarouselImageLink(scope, element, attributes, backgroundMediacarouselCtrl) {
            scope.timeoutInterval = backgroundMediacarouselCtrl.getTimeoutInterval();

            /**
             * Notify is called when active index changes in carousel.
             * @param index new active index
             * @returns {Promise} returns a promise when rotation is complete.
             */
            scope.notify = function (index) {

                scope.isOnTop = (scope.assignedIndex === index);
                scope.$digest();

                //create a promise and resolve when done.
                return new Promise(function (resolve) {
                    if (scope.isOnTop) { //disable rotation for small screens
                        scope.timerHandle = setTimeout(resolve, scope.timeoutInterval);
                    } else {
                        resolve();
                    }
                });
            };

            function init() {
                scope.assignedIndex = backgroundMediacarouselCtrl.registerCarouselItem(scope.notify);
            }

            // watch ocdPath
            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE,scope.ocdPath)
                .then(init);

        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                ocdPath: '@',
                imageUrl: '@'
            },
            require: '^originBackgroundMediacarousel',
            link: originBackgroundMediacarouselImageLink,
            controller: 'originBackgroundMediacarouselItemCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/background-mediacarouselitem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originBackgroundMediacarouselImage
     * @restrict E
     * @element ANY
     * @scope
     * @param {OcdPath} ocd-path OCD Path
     * @param {ImageUrl} image-url url of the background image
     * @description Displays a packArt and background image.
     *
     * @example
     * <origin-store-row>
     *     <origin-background-mediacarousel>
     *          <origin-background-mediacarousel-image ocd-path="asdasfsa" image-url="asdasdasd"></origin-background-mediacarousel-image>
     *     </origin-background-mediacarousel>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('originBackgroundMediacarouselItemCtrl', originBackgroundMediacarouselItemCtrl)
        .directive('originBackgroundMediacarouselImage', originBackgroundMediacarouselImage);
}());
