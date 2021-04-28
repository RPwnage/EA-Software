/**
 * @file scripts/blur-image-ogd.js
 */
(function () {
    'use strict';
    function originBlurImageOgd(ComponentsConfigFactory, FeatureDetectionFactory, CSSUtilFactory, ScreenFactory) {

        var DEFAULT_BLUR_VALUE = null,
            TEMPLATES = {
                css: {
                    templateUrl: ComponentsConfigFactory.getTemplatePath('views/blur-image-ogd.html')
                },
                svg: {
                    templateUrl: ComponentsConfigFactory.getTemplatePath('views/blur-image-svg.html')
                }
            },
            BACKGROUND_SCRIMS_SELECTOR = '[origin-store-backgroundscrims]',
            BLUR_SVG_ACTIVE_CLASS = 'origin-store-blursvg-active';

        function supportsCSSFilters() {
            return FeatureDetectionFactory.browserSupports('css-filters');
        }

        function originBlurImageOgdLink(scope, element) {

            scope.blurStyle = {};

            function watchBlurValue(callback) {
                scope.$watch('blurValue', function (newValue) {
                    if (angular.isDefined(newValue) && !ScreenFactory.isXSmall()) {
                        callback(newValue);
                    } else {
                        callback(DEFAULT_BLUR_VALUE);
                    }
                });
            }

            function blurWithCSS(newValue) {
                if (newValue) {
                    scope.blurStyle = CSSUtilFactory.addVendorPrefixes('filter', 'blur(' + newValue + 'px)');
                } else {
                    scope.blurStyle = {};
                }
            }

            function blurWithSVG(newValue) {
                if (!newValue) {
                    newValue = 0;
                }
                var svgFilter = angular.element('.origin-blurredimage-svg-stddeviation');
                if (svgFilter.length) {
                    svgFilter[0].setStdDeviation(newValue, newValue);
                }
            }

            if (supportsCSSFilters()) {
                watchBlurValue(blurWithCSS);
            } else if (FeatureDetectionFactory.browserSupports('svg-basicfilter')) {
                element.parents(BACKGROUND_SCRIMS_SELECTOR).addClass(BLUR_SVG_ACTIVE_CLASS);
                watchBlurValue(blurWithSVG);
            }
        }

        /**
         * The blur image directive partial
         * @return {Object} the directive without a template path
         */
        function getBlurImageOgdDirective() {
            return {
                restrict: 'E',
                replace: true,
                scope: {
                    imageUrl: '=',
                    blurValue: '=',
                    imageOptions: '=',
                    svgOptions: '='
                },
                link: originBlurImageOgdLink
            };
        }

        /**
         * To support embedded templates, the app can choose between template and templateUrl values
         * for the directive. This function will decide which one to provide based on CSS filter availability
         * @return {Object} the correct template path
         */
        function getTemplate() {
            if(supportsCSSFilters()) {
                return TEMPLATES.css;
            } else {
                return TEMPLATES.svg;
            }
        }

        return _.merge(getBlurImageOgdDirective(), getTemplate());

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originBlurImageOgd
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {imageUrl} image-url url of the image to blur
     * @param {integer} blur-value value from 0 to 10
     * @param {Object} image-options (optional) image params
     * @param {Object} svg-options (optional) svg attributes and values
     * @description
     *
     * An image blurring mechanism specifically for the ogd flyout (omits the origin fade in directive)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-blur-image image-url="" blur-value="1" image-options="{x: '-60%', h: '0', width: '200%', height: '200%'}"/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originBlurImageOgd', originBlurImageOgd);
}());
