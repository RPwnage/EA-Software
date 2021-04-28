/**
 * @file store/pdp/feature/premier.js
 */
(function () {
    'use strict';

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
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

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-feature-premier-wrapper';
    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-premier';

    function originStorePdpFeaturePremier(ComponentsConfigFactory, DirectiveScope, ScreenFactory, AnimateFactory) {

        function originStorePdpFeaturePremierLink(scope, element, attrs, originStorePdpSectionWrapperCtrl) {

            scope.anyDataFound = false;

            function updateVisibility() {
                scope.isTextInView = ScreenFactory.isOnOrAboveScreen(element);
                scope.$digest();
            }

            function init(anyDataFound) {
                scope.anyDataFound = anyDataFound;
                if (anyDataFound) {
                    scope.isRight = (scope.textAlign === TextAlignEnumeration.Right);
                    scope.isLeft = (scope.textAlign === TextAlignEnumeration.Left);
                    scope.isCenter = (!scope.isRight && !scope.isLeft);
                    scope.isDarkColor = (scope.textColor === TextColorsEnumeration.dark);

                    if (!ScreenFactory.isXSmall()) {
                        scope.isTextInView = ScreenFactory.isOnOrAboveScreen(element);
                        scope.textSlide = BooleanEnumeration[scope.textSlide] === 'true';
                        scope.tSlide = BooleanEnumeration[scope.textSlide] === 'true';
                        AnimateFactory.addScrollEventHandler(scope, updateVisibility);
                    } else {
                        scope.animationDisabled = true;
                    }

                    if (originStorePdpSectionWrapperCtrl && _.isFunction(originStorePdpSectionWrapperCtrl.setVisibility)){
                        originStorePdpSectionWrapperCtrl.setVisibility(true);
                    }

                    scope.$digest();
                }
            }

            DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE]).then(init);
        }

        return {
            require: '^?originStorePdpSectionWrapper',
            restrict: 'E',
            scope: {
                image: '@',
                header: '@',
                text: '@',
                textAlign: '@',
                textColor: '@',
                textSlide: '@'
            },
            link: originStorePdpFeaturePremierLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/premier.html')
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
     * @param {BooleanEnumeration} text-slide should text slide or no
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
