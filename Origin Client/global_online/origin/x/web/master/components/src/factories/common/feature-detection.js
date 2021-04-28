/**
 * @file factories/common/feature-detection.js
 */
(function() {
    'use strict';

    function FeatureDetectionFactory(CSSUtilFactory) {

        function supportsCSSFilters() {
            //logic is taken from https://raw.githubusercontent.com/Modernizr/Modernizr/master/feature-detects/css/filters.js
            var el = document.createElement('a'),
                cssObject = CSSUtilFactory.addVendorPrefixes('filter', 'blur(2px)');

            el.style.cssText = convertCSSObjectToString(cssObject);
            return !!el.style.length && ((angular.isUndefined(document.documentMode) || document.documentMode > 9));
        }

        var detectionStrategies = {
            'css-filters': supportsCSSFilters
        };

        function convertCSSObjectToString(cssObject) {
            var cssText = '';
            for (var key in cssObject) {
                cssText += key + ': ' + cssObject[key] + ';';
            }
            return cssText;
        }

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
