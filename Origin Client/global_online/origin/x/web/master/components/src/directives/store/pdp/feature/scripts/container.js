
/**
 * @file store/pdp/feature/scripts/container.js
 */
(function(){
    'use strict';

    var DELAY = 100;

    /**
      * A list of text colors
      * @readonly
      * @enum {string}
      */
     var TextColorsEnumeration = {
         dark: 'dark',
         light: 'light'
     };

    function originStorePdpFeatureContainer(ComponentsConfigFactory, CSSUtilFactory, $timeout, UtilFactory) {

        function originStorePdpFeatureContainerLink(scope, elem) {
            scope.moduleBackgroundColor = CSSUtilFactory.createBackgroundColor(scope.backgroundColor);
            scope.moduleAccentColor = CSSUtilFactory.createBackgroundColor(scope.accentColor);
            scope.isDarkColor = (scope.textColor === TextColorsEnumeration.dark);

            var featureElems = elem.find('.origin-pdp-feature-bubble');

            function checkBubbles() {
                for (var i=0; i < featureElems.length; i++) {
                    var currElem = angular.element(featureElems.get(i));
                    if(isBubbleVisible(currElem)) {
                        $timeout(setBubbleVisible(currElem), i * DELAY);
                    } else {
                        currElem.removeClass('origin-pdp-feature-slideUp');
                    }
                }
            }

            function isBubbleVisible(elem){
                return UtilFactory.isOnScreen(elem) || UtilFactory.isAboveScreen(elem);
            }

            function setBubbleVisible(elem) {
                return function(){
                    elem.addClass('origin-pdp-feature-slideUp');
                };
            }

            angular.element(window).on("scroll", checkBubbles);

        }
        return {
            restrict: 'E',
            scope: {
                title: '@',
                text: '@',
                textColor: '@',
                backgroundColor: '@',
                accentColor: '@'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/container.html'),
            link: originStorePdpFeatureContainerLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureContainer
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title (optional) key feature container title copy
     * @param {LocalizedString} text (optional) key feature container text copy
     * @param {string} background-color module background color
     * @param {string} accent-color module accent color (used for divider line)
     * @description
     *
     * Container to hold the key feature items (bubbles)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-feature-container title="Five Reasons to Play" text="Explore a vast range of experiences that allow you to play your way." />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureContainer', originStorePdpFeatureContainer);
}());
