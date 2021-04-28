/**
 * @file store/pdp/feature/premier.js
 */
(function(){
    'use strict';

    var TextSlideEnumeration = {
        "true": true,
        "false": false
    };

    /**
     * list text alignment types
     * @readonly
     * @enum {string}
     */
    var TextAlignEnumeration = {
        "Left": "left",
        "Center": "center",
        "Right": "right"
    };

    /**
     * A list of text colors
     * @readonly
     * @enum {string}
     */
    var TextColorsEnumeration = {
        "dark": "dark",
        "light": "light"
    };

    function originStorePdpFeaturePremier(ComponentsConfigFactory, UtilFactory) {

        function originStorePdpFeaturePremierLink(scope, element) {

            function updateVisibility() {
                scope.isTextInView = UtilFactory.isOnScreen(element) || UtilFactory.isAboveScreen(element);
                scope.$digest();
            }

            scope.isRight = (scope.textAlign === TextAlignEnumeration.Right);
            scope.isLeft = (scope.textAlign === TextAlignEnumeration.Left);
            scope.isCenter = (!scope.isRight && !scope.isLeft);
            scope.isDarkColor = (scope.textColor === TextColorsEnumeration.dark);
            scope.isTextInView = UtilFactory.isOnScreen(element) || UtilFactory.isAboveScreen(element);

            angular.element(window).on('scroll', updateVisibility);
            scope.textSlide = TextSlideEnumeration[scope.textSlide];
            scope.tSlide = TextSlideEnumeration[scope.textSlide];
        }

        return {
            restrict: 'E',
            scope: {
                image: '@',
                header: '@',
                text: '@',
                textAlign: '@',
                textColor: '@',
                textSlide: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/premier.html'),
            link: originStorePdpFeaturePremierLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeaturePremier
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} image background image URL
     * @param {LocalizedString} header (optional) header text
     * @param {LocalizedString} text (optional) main paragraph
     * @param {TextAlignEnumeration} text-align text alignment
     * @param {TextColorsEnumeration} text-color text color: light or dark
     *
     * Large feature image PDP section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-premier
     *             text-color="light"
     *             text-slide="true"
     *             image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png"
     *             header="Change the face of battle with LEVOLUTION & more"
     *             text="From the big to the small, the dynamic environment is affected by your actions—and the consequences change the landscape of gameplay in breathtaking fashion.">
     *         </origin-store-pdp-feature-premier>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeaturePremier', originStorePdpFeaturePremier);
}());
