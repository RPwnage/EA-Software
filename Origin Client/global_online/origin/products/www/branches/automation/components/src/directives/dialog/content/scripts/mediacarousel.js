
/**
 * @file dialog/content/scripts/mediacarousel.js
 */
(function(){
    'use strict';
    var CHAR_CODE_LEFT_ARROW = 37;
    var CHAR_CODE_RIGHT_ARROW = 39;

    function originDialogContentMediaCarousel(ComponentsConfigFactory) {
        function originDialogContentMediaCarouselLink(scope) {
            var keyHandler = {};

            scope.active = parseInt(_.get(scope, 'active', 0), 10);

            scope.previousMediaItem = function() {
                var activeSlide = parseInt(scope.active, 10);

                if(activeSlide === 0) {
                    scope.active = _.size(scope.items) - 1;
                } else {
                    scope.active = activeSlide - 1;
                }

                updateMediaType();

                scope.$broadcast('dialogContent:mediaCarousel:mediaChanged');

            };

            scope.nextMediaItem = function() {
                var activeSlide = parseInt(scope.active, 10);

                if(activeSlide === _.size(scope.items) - 1) {
                    scope.active = 0;
                } else {
                    scope.active = activeSlide + 1;
                }

                updateMediaType();

                scope.$broadcast('dialogContent:mediaCarousel:mediaChanged');
            };

            function updateMediaType(){
                scope.isTypeVideo = _.get(scope, ['items', scope.active, 'type']) === 'video';
            }

            keyHandler[CHAR_CODE_LEFT_ARROW] = scope.previousMediaItem;
            keyHandler[CHAR_CODE_RIGHT_ARROW] = scope.nextMediaItem;

            /**
             * Handles left and right arrow key navigation between carousel items
             * @param {KeyEvent} e
             */
            function processKeyStroke(e){
                var charCode = e.keyCode || e.which;
                if (keyHandler[charCode]){
                    keyHandler[charCode]();
                    e.stopPropagation();
                    scope.$digest();
                }
            }

            function unbindKeyEvent(){
                document.removeEventListener('keyup', processKeyStroke);
            }
            document.addEventListener('keyup', processKeyStroke);
            scope.$on('$destroy', unbindKeyEvent);

            updateMediaType();
        }

        return {
            restrict: 'E',
            scope: {
                items: '=',
                active: '='
            },
            link: originDialogContentMediaCarouselLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/mediacarousel.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentMediaCarousel
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} images An array of MediaCarousel
     * @param {string} active The index of the active screenshot
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-content-media-carousel />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentMediaCarousel', originDialogContentMediaCarousel);
}());
