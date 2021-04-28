
/**
 * @file store/pdp/sections/scripts/background.js
 */
(function() {
    'use strict';
    function originStorePdpSectionsBackground(ComponentsConfigFactory, ScreenFactory, AnimateFactory) {

        function originStorePdpSectionsBackgroundLink(scope, elem) {
            scope.blurValue = 0;
            scope.bottomScrim = 'true';
            
            function applyBlur(element) {

                var wtop = angular.element(window).scrollTop(),
                    etop = element.offset().top,
                    ebottom = etop + element.outerHeight(),
                    maxBlur = 30;

                // maths
                var visiblePercentage = Math.min(1, (ebottom - wtop) / (ebottom - etop));

                scope.blurVal = Math.floor((1 - visiblePercentage) * maxBlur);
                scope.$digest();
            }

            function onScroll() {
                var element = elem.find('.origin-pdp-hero-background');
                if(ScreenFactory.isOnScreen(element)) {
                    applyBlur(element);
                }
            }

            AnimateFactory.addScrollEventHandler(scope, onScroll);

        }

        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/background.html'),
            link: originStorePdpSectionsBackgroundLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsBackground
     * @restrict E
     * @element ANY
     * @scope
     * @description

     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-sections-background />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpSectionsBackground', originStorePdpSectionsBackground);
}());
