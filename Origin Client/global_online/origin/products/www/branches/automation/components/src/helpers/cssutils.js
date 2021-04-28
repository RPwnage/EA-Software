/**
 * @file common/cssutil.js
 */
(function() {
    'use strict';

    var HEX_SHORTHAND_REGEX = /([a-f\d])([a-f\d])([a-f\d])/i;
    var BACKGROUND_COLOR_KEY = 'background-color';

    function CSSUtilFactory(CSSGradientFactory, LocDictionaryFactory) {

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

        /**
         * Convert a CSS ruleset object to a string
         * @param  {Object} CSS rules as key/value pairs
         * @return {String}
         */
        function convertCSSObjectToString(cssObject) {
            var cssText = '';
            _.forEach(cssObject, function(val, key) {
                cssText += key + ': ' + val + ';';
            });
            return cssText;
        }

        function createBackgroundColor(backgroundColor) {
            return {
                "background-color": backgroundColor
            };
        }

        /**
         * Converts a hex color into an rgba color.
         * @param  {string} hex     Hex color with or without a hash
         * @param  {float} opacity  Opacity value between 0 - 1. Optional, defaults to 1.
         * @return {string}         rgba formatted string, undefined if invalid hex value given
         */
        function hexToRgba(hex, opacity) {

            if (!angular.isDefined(hex)) {
                return;
            } else if (hex.indexOf('rgb') >= 0) {
                // Return value if already in rgb format
                return hex;
            }

            // remove hash if given
            hex = hex.replace('#', '');

            // Explode hex if shorthand given (e.g. "FFF" to "FFFFFF")
            if (hex.length === 3){
                hex = hex.replace(HEX_SHORTHAND_REGEX, function(match, r, g, b) {
                    return r + r + g + g + b + b;
                });
            } else if (hex.length !== 6) { // If length of hex isn't 3 or 6, return undefined for invalid hex
                return;
            }

            // convert from hexadecimal to base 10
            var r = parseInt(hex.substring(0,2), 16);
            var g = parseInt(hex.substring(2,4), 16);
            var b = parseInt(hex.substring(4,6), 16);
            opacity = (_.isNumber(opacity) && (opacity >= 0) && (opacity <= 1)) ? opacity : 1;

            if (isNaN(r) || isNaN(g) || isNaN(b)){
                return;
            } else {
                return 'rgba(' + [r, g, b, opacity].join(',') + ')';
            }
        }

        /**
         * Set the background color of a given element. Uses the scope of the calling directive to watch on changes 
         * of background color for when CQ loads merchandised background colors. Otherwise looks up default background 
         * color for that component from the dictionary. 
         * @param {object} backgroundColor  The background color to be set
         * @param {object} element          The angular element that needs the background color set 
         * @param {string} contextNamespace The context namspace for the component. Used for dictionary lookup of the default color.
         * @return {string} The new background color of the element (in case overridden with default)
         */
        function setBackgroundColorOfElement(backgroundColor, element, contextNamespace) {
            var defaultBackgroundColor = LocDictionaryFactory.getDefaultValue(BACKGROUND_COLOR_KEY, contextNamespace);

            backgroundColor = backgroundColor && backgroundColor.length ? backgroundColor : defaultBackgroundColor;
            if (backgroundColor) {
                element.css('background-color', hexToRgba(backgroundColor));
            }

            return backgroundColor;
        }


        return {
            createLinearGradient: CSSGradientFactory.createLinearGradient,
            createBackgroundColor: createBackgroundColor,
            addVendorPrefixes: addVendorPrefixes,
            convertCSSObjectToString: convertCSSObjectToString,
            hexToRgba: hexToRgba,
            setBackgroundColorOfElement: setBackgroundColorOfElement
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.CSSUtilFactory

     * @description
     *
     * UtilFactory
     */
    angular.module('origin-components')
        .factory('CSSUtilFactory', CSSUtilFactory);

})();