
/**
 * @file scripts/header-image.js
 */
(function() {
    'use strict';
    function originProfileHeaderImage(ComponentsConfigFactory, FeatureDetectionFactory, CSSUtilFactory) {

        var TEMPLATES = {
                css: {
                    templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/header-image.html')
                },
                svg: {
                    templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/header-image-svg.html')
                }
            };

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
                scope.blurStyle = CSSUtilFactory.addVendorPrefixes('filter', 'blur(30px)');
            }

            function blurWithSVG() {
                filterElement[0].setStdDeviation(30, 30);
            }

            if (supportsCSSFilters()) {
                blurWithCSS(scope.blurValue);
            } else if (supportsSVGBlurFilter()) {
                blurWithSVG(scope.blurValue);
            }
        }

        /**
         * The header image directive partial
         * @return {Object} the directive without a template path
         */
        function getHeaderImageDirective() {
            return {
                restrict: 'E',
                replace: true,
                scope: {
                    imageUrl: '='
                },
                link: originProfileHeaderImageLink
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

        return _.merge(getHeaderImageDirective(), getTemplate());
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
