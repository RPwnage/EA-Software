(function() {
    'use strict';

    function PluralizeFactory() {
        /**
        * Trim the whitespace of the string
        * @param {String} str - the string to trim
        * @return {string} - the trimmed string
        */
        function trim(str) {
            if (str.trim) {
                return str.trim();
            } else {
                return str.replace(/^\s+|\s+$/gm,'');
            }
        }

        /**
        * Get the lower bound for the range
        * @param {String} leftParenthesis - the left paraenthesis which may be a ] or [ which
        *       determines if the range is bounded or unbounded
        * @param {String} leftVal - the value of the lower range
        * @return {Number} - the lower bound
        */
        function getLowerBound(leftParenthesis, leftVal) {
            if (leftVal === '-Inf') {
                return -1 * Infinity;
            }
            return (leftParenthesis === ']') ? 1 + parseInt(leftVal, 10) : parseInt(leftVal, 10);
        }

        /**
        * Get the upper bound for the range
        * @param {String} rightParenthesis - the right paraenthesis which may be a ] or [ which
        *       determines if the range is bounded or unbounded
        * @param {String} rightVal - the value of the upper range
        * @return {Number} - the upper bound
        */
        function getUpperBound(rightParenthesis, rightVal) {
            if (rightVal === '+Inf') {
                return Infinity;
            }
            return (rightParenthesis === '[') ? parseInt(rightVal, 10) - 1 : parseInt(rightVal, 10);
        }

        /**
        * Determine the upper and lower bounds for a range string
        * @param {String} str - the pluralization string
        * @return {Object} - object with the upper and lower bounds
        */
        function getUpperAndLowerBounds(str) {

            var rangeMatch = str.match(/^([\[\]])\s*(-Inf|-?\d+)\s*,\s*(\+Inf|-?\d+)\s*([\[\]])\s*/),
                lBound, uBound;

            if (!rangeMatch || rangeMatch.length !== 5) {
                return false;
            }

            // get the upperBound and the lower bound
            lBound = getLowerBound(rangeMatch[1], rangeMatch[2]);
            uBound = getUpperBound(rangeMatch[4], rangeMatch[3]);

            // parameters are corrupt, return false
            if (isNaN(lBound) || isNaN(uBound) || lBound > uBound) {
                return false;
            }

            return {
                'lowerBound': lBound,
                'upperBound': uBound,
                'matchedStr': rangeMatch[0]
            };

        }

        /**
        * Decompose a string that has a bounded range into the string
        * that should be used if the delimiting number is within the bounded
        * range and a function that determines if the delimiting number is
        * indeed within that range.
        * @param {String} str - the pluralization segement string
        * @param {Array} arr - the decomposition array which we add to
        * @return {Boolean} - true if the string was successfully decomposed into a range match
        */
        function decomposeRange(str, arr) {
            var bounds = getUpperAndLowerBounds(str);
            if (bounds) {
                arr.push({
                    'str': str.replace(bounds.matchedStr, ''),
                    'matches': (function(low, up) {
                        return function(val) {
                            return (val >= low) && (val <= up);
                        };
                    }(bounds.lowerBound, bounds.upperBound))
                });
                return true;
            }
            return false;
        }

        /**
        * Decompose a string that is to be split on a specific number (or numbers)
        * into the resulting string that it should resolve to and a matching
        * function which determines if a number matches the number
        * for that string.
        * @param {String} str - the pluralization segement string
        * @param {Array} arr - the decomposition array which we add to
        * @return {Boolean} - true if the string was successfully decomposed into a numeric match
        */
        function decomposeNumeric(str, arr) {
            var numMatch = str.match(/^\{\s*(-?\d+[\s*,\s*-?\d+]*)\s*\}\s*/);
            if (numMatch) {
                arr.push({
                    'str': str.replace(numMatch[0], ''),
                    'matches': (function(nums) {
                        return function(val) {
                            for (var i=0, j=nums.length; i<j; i++) {
                                if (parseInt(nums[i], 10) === val) {
                                    return true;
                                }
                            }
                            return false;
                        };
                    }(numMatch[0].replace(/\{|\}\s/g, '').split(',')))
                });
                return true;
            }
            return false;
        }

        /**
        * Decompose a pluralization string into a series of
        * strings with matching functions which will then be used
        * to determine if the delimiting number matches the pluralization
        * string.  If so, it will give us the string to use.
        * @param {Array} str - the pluralization segment array
        * @return {Array} - the decomposition of the pluralization segment into matching functions to test
        */
        function decompose(strs) {
            var decomposition = [];
            for (var i = 0, j = strs.length; i < j; i++) {
                // trim the string first
                var str = trim(strs[i]);
                var isNumDecomposition = decomposeNumeric(str, decomposition);
                if (!isNumDecomposition) {
                    decomposeRange(str, decomposition);
                }
            }
            return decomposition;
        }

        /**
        * Pluralization resolution can take the following formats:
        *
        * '{num} a string goes here'
        * '{num,num,num} a string goes here'
        *
        * ']-Inf, num] a string goes here'
        * '[lowNum, highNum] a string goes here'
        * '[lowNum, +Inf[ a string goes here'
        *
        * 'one: a string goes here'
        * 'many: a string goes here'
        *
        * @param {String} str - the translated string in pluralization form
        * @param {Number} delimNum - the number used to resolve the pluralization
        * @return {String} - the string to use after the pluralization is resolved
        */
        function resolve(str, delimNum) {
            var pluralizedPieces = str.split('|');

            delimNum = Number(delimNum);
            if(!isFinite(delimNum)) {
                delimNum = 0;
            }

            if (pluralizedPieces.length <= 1) {
                return str;
            }

            // only resolve if we need to.  If we are just
            // passed a string then don't worry about it.
            var pluralVariants = decompose(pluralizedPieces);
            for (var i = 0, j = pluralVariants.length; i < j ; i++) {
                if (pluralVariants[i].matches(delimNum)) {
                    return pluralVariants[i].str;
                }
            }

            // no match found
            throw 'No match found';
        }

        return {
            resolve: resolve
        };
    }

    angular.module('origin-i18n')
        .factory('PluralizeFactory', PluralizeFactory);
}());
