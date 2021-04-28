/**
 * @file common/cssutil.js
 */
(function() {
    'use strict';

    function CSSUtilFactory() {
        /**
         * Adds vendor-specific prefixes to the given CSS property
         *
         * @param {string} property The property name
         * @param {string} property The property value
         * @return {Object}
         */
        function addVendorPrefixes(property, value) {
            var cssBlock = {};

            cssBlock[property] = value;
            cssBlock['-webkit-' + property] = value;
            cssBlock['-moz-' + property] = value;
            cssBlock['-ms-' + property] = value;

            return cssBlock;
        }

        return {
            /**
             * @todo: this function has migrated to CSSGradientFactory. Remove it from here
             * Return a linear gradient from left to right
             *
             * @param  {String} startColor      A string representation of an rgba color eg rgba(30, 38, 44, 0)
             * @param  {Number} startPosPercent Where to start the gradient at as a percentage
             * @param  {String} endColor        A string representation of an rgba color eg rgba(30, 38, 44, 0)
             * @param  {Number} endPosPercent   Where to end the gradient at as a percentage
             * @return {object}                 the css to be applied as an object possibly containing array.
             */
            createLinearGradient: function (startColor, startPosPercent, endColor, endPosPercent) {
                return  {
                    "background-image": [
                        "-webkit-linear-gradient(left, color-stop(" + endColor +" " + startPosPercent + "%), color-stop(" + startColor + " " + endPosPercent + "%))", // Safari 5.1-6, Chrome 10+
                        "linear-gradient(to right, " + endColor + " " + startPosPercent + "%, " + startColor + " " + endPosPercent + "%)" // Standard, IE10, Firefox 16+, Opera 12.10+, Safari 7+, Chrome 26+
                    ],
                    "background-repeat": "repeat-x",
                    "filter": "progid:DXImageTransform.Microsoft.gradient(startColorstr='" + endColor + "', endColorstr='" + startColor + "', GradientType=1)" // IE9 and down
                };
            },

            createBackgroundColor: function(backgroundColor) {
                return {
                    "background-color": backgroundColor
                };
            },

            addVendorPrefixes: addVendorPrefixes
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function CSSUtilFactorySingleton(LocFactory, ComponentsLogFactory, $http, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('CSSUtilFactory', CSSUtilFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.CSSUtilFactory

     * @description
     *
     * UtilFactory
     */
    angular.module('origin-components')
        .factory('CSSUtilFactory', CSSUtilFactorySingleton);

})();