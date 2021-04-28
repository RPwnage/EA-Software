
/**
 * @file scripts/header-image.js
 */
(function() {
    'use strict';
    function originProfileHeaderImage(ComponentsConfigFactory, FeatureDetectionFactory, CSSUtilFactory) {

        function supportsCSSFilters() {
            return FeatureDetectionFactory.browserSupports('css-filters');
        }

        function originProfileHeaderImageLink(scope) {
            var filterElement = $('.origin-profile-header-image-svg-stddeviation');

            function supportsSVGBlurFilter() {
                return !!filterElement && filterElement.length && angular.isDefined(filterElement[0].setStdDeviation);
            }


            /* Blur methods */
            function blurWithCSS() {
                scope.blurStyle = CSSUtilFactory.addVendorPrefixes('filter', 'blur(15px)');
            }

            function blurWithSVG() {
                filterElement[0].setStdDeviation(15, 15);
            }

            if (supportsCSSFilters()) {
                blurWithCSS(scope.blurValue);
            } else if (supportsSVGBlurFilter()) {
                blurWithSVG(scope.blurValue);
            }

        }

        return {
            restrict: 'E',
            replace: true,
            templateUrl: function () {
                var templateUrl;
                if (supportsCSSFilters()) {
                    templateUrl = 'profile/views/header-image.html';
                } else {
                    templateUrl = 'profile/views/header-image-svg.html';
                }
                return ComponentsConfigFactory.getTemplatePath(templateUrl);
            },
            scope: {
                imageUrl: '='
            },
            link: originProfileHeaderImageLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileHeaderImage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {imageUrl} image-url url of the image to blur
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-profile-header-image image-url="" />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originProfileHeaderImage', originProfileHeaderImage);
}());
