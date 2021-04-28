/**
 * @file factories/common/feature-detection.js
 */
(function() {
    'use strict';

    function FeatureDetectionFactory(CSSUtilFactory) {

        /**
         * Given a CSS rule and test value, return whether the browser successfully applies this to a test DOM element
         * @param  {String} ruleName rule to test support for
         * @param  {String} ruleValue value to use for the specified rule
         * @return {boolean}
         */
        function supportsCSSFeature(ruleName, ruleValue) {
            //logic is taken from https://raw.githubusercontent.com/Modernizr/Modernizr/master/feature-detects/css/filters.js
            var el = document.createElement('a'),
                cssObject = CSSUtilFactory.addVendorPrefixes(ruleName, ruleValue);

            el.style.cssText = CSSUtilFactory.convertCSSObjectToString(cssObject);
            return !!el.style.length && ((angular.isUndefined(document.documentMode) || document.documentMode > 9));
        }

        function supportsMsAccelerator() {
            // ms-accelerator is only supported in <= v13. Using this to detect versions older than v13.
            return CSS.supports("-ms-accelerator", true);
        }

        function supportsSVGBasicFilter() {
            return document.implementation.hasFeature('http://www.w3.org/TR/SVG11/feature#BasicFilter', '1.1');
        }

        function supportsCSSFilters() {
            return supportsCSSFeature('filter', 'blur(2px)');
        }

        function supportsLineClamp() {
            return supportsCSSFeature('line-clamp', '2');
        }

        var detectionStrategies = {
            'css-filters': supportsCSSFilters,
            'line-clamp': supportsLineClamp,
            'svg-basicfilter': supportsSVGBasicFilter,
            'ms-accelerator': supportsMsAccelerator
        };

        function supports(feature) {
            if (feature) {
                var isSupported = detectionStrategies[feature];
                return isSupported && isSupported();
            }
            return false;
        }

        return {
            features: Object.keys(detectionStrategies),
            browserSupports: supports
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function FeatureDetectionFactorySingleton(CSSUtilFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('FeatureDetectionFactory', FeatureDetectionFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.FeatureDetectionFactory
     * @description
     *
     * Object manipulation functions
     */
    angular.module('origin-components')
        .factory('FeatureDetectionFactory', FeatureDetectionFactorySingleton);
}());
