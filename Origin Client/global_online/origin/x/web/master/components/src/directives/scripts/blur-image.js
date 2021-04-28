
/**
 * @file scripts/blur-image.js
 */
(function() {
    'use strict';
    function originBlurImage(ComponentsConfigFactory, FeatureDetectionFactory, CSSUtilFactory) {

        var SVG_DEFAULTS = {
            x: '-120',
            y: '-20',
            width: '120%',
            height: '120%'
        };

        function supportsCSSFilters() {
            return FeatureDetectionFactory.browserSupports('css-filters');
        }

        function originBlurImageLink(scope) {

            var filterElement = $('.origin-blurredimage-svg-stddeviation');
            scope.blurStyle = {};

            //apparently much faster than angular.extend
            //https://jsperf.com/jquery-extend-vs-angular-extend-vs-lodash-extend/2
            scope.options = $.extend({}, SVG_DEFAULTS, scope.imageOptions);

            function supportsSVGBlurFilter() {
                return filterElement && filterElement.length && angular.isDefined(filterElement[0].setStdDeviation);
            }

            function watchBlurValue(callback) {
                scope.$watch('blurValue', function (newValue) {
                    if (angular.isDefined(newValue)) {
                        callback(newValue);
                    }
                });
            }

            function blurWithCSS(newValue) {
                scope.blurStyle = CSSUtilFactory.addVendorPrefixes('filter', 'blur(' + newValue + 'px)');
            }

            function blurWithSVG(newValue) {
                filterElement[0].setStdDeviation(newValue, newValue);
            }

            if (supportsCSSFilters()) {
                watchBlurValue(blurWithCSS);
            } else if (supportsSVGBlurFilter()) {
                watchBlurValue(blurWithSVG);
            }
        }

        return {
            restrict: 'E',
            replace: true,
            templateUrl: function () {
                var templateUrl;
                if (supportsCSSFilters()) {
                    templateUrl = 'views/blur-image.html';
                } else {
                    templateUrl = 'views/blur-image-svg.html';
                }
                return ComponentsConfigFactory.getTemplatePath(templateUrl);
            },
            scope: {
                imageUrl: '=',
                blurValue: '=',
                imageOptions: '='
            },
            link: originBlurImageLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originBlurImage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {imageUrl} image-url url of the image to blur
     * @param {integer} blur-value value from 0 to 10
     * @param {Object} image-options (optional) SVG params
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-blur-image image-url="" blur-value="1" image-options="{x: '-60%', h: '0', width: '200%', height: '200%'}"/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originBlurImage', originBlurImage);
}());
