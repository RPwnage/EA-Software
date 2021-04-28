/**
 * @file store/pdp/feature/video.js
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

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "none": "",
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };

    function originStorePdpFeatureVideo(ComponentsConfigFactory, UtilFactory) {

        function originStorePdpFeatureVideoLink(scope, element) {

            function updateVisibility() {
                scope.isTextInView = UtilFactory.isOnScreen(element) || UtilFactory.isAboveScreen(element);
                scope.$digest();
            }

            scope.ctaType = scope.ctaType || ButtonTypeEnumeration.none;
            scope.isExternalCta = (scope.ctaHref.length > 4 && scope.ctaHref.substr(0, 4) === 'http');

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
                video: '@',
                header: '@',
                text: '@',
                textAlign: '@',
                textColor: '@',
                textSlide: '@',
                ctaType: '@',
                ctaDescription: '@',
                ctaHref: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/video.html'),
            link: originStorePdpFeatureVideoLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureVideo
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} video youtube id of video
     * @param {TextColorsEnumeration} text-color text color: light or dark
     * @param {TextAlignEnumeration} text-align text alignment
     * @param {TextSlideEnumeration} text-slide boolean to determine if text slides in or not
     * @param {LocalizedString} header (optional) header text
     * @param {LocalizedString} text (optional) main paragraph
     * @param {ButtonTypeEnumeration} cta-type type of CTA button
     * @param {LocalizedString} cta-description CTA description text
     * @param {Url} cta-href URL of CTA link
     * @description
     *
     * Large feature video PDP section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-video
     *         video="3eVF9uBbuqc"
     *         text-color="light"
     *         text-align="center"
     *         text-slide="true"
     *         header="Immerse yourself in your Star Wars Battlefront fantasies"
     *         text="Photorealistic visuals and authentic sound design from the talented team at DICE. Prepate to be transported to a galaxy far, far away."
     *         cta-type="transparent"
     *         cta-description="Watch the E3 Gameplay Trailer"
     *         cta-href="#"
     *     ></origin-store-pdp-feature-video>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureVideo', originStorePdpFeatureVideo);
}());
