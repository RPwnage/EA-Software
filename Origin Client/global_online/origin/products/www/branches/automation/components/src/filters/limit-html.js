/**
 * @file limit-html.js
 */

(function() {
    'use strict';

    /**
     * @ngdoc filter
     * @name origin-components.filters:limitHtml
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Trims a string containing html to the first end tag or trim length (limit) whichever comes first, 
     * then sanitizes the string by removing all html.
     * Once trim location has been found, it searches back from this location and trims to the nearest sentence end. 
     * If a sentence is not found it searches back to the nearest word or punctuation mark and appends
     * an optional truncation string ie '...'
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <span>{{ title | limitHtml:100:'ellipsis' }}</span>
     *     </file>
     * </example>
     *
     */
    var REPLACEMENT_CHAR = '~~', // arbitrary unique string to signify an end tag
        // matches a non inline end tag
        REGEX_END_TAG = /<\/(?!(a>|b>|br|abbr|acronym|strong|cite|code|em|time|samp|sub|sup|q>|var|input|label|select|textarea|button|i>|img|span))[^>]+>/gm,
        REGEX_END_TAG_WITH_BR = /<(?:br\/*)>|<\/(?!(a>|b>|br|abbr|acronym|strong|cite|code|em|time|samp|sub|sup|q>|var|input|label|select|textarea|button|i>|img|span))[^>]+>/gm,
        REGEX_HTML_TAG = /<[^>]+>/gm, // any html tag
        REGEX_ENDING_PUNCTUATION = /([\.\?!。])/g, // a character that signifies the end of a sentence or group
        REGEX_NON_ENDING_PUNCTUATION = /([,:\;\-])/g,
        REGEX_NBSP = /\u00a0/g;

    function limitHtml() {
        // Trims the the last space in the string. Does nothing if no spaces found.
        function trimToLastSpace(str) {
            var indexOfLastSpace = str.lastIndexOf(' ');
            
            return indexOfLastSpace > 0 ? str.substr(0, indexOfLastSpace) : str;
        }

        // Returns the text value of the first block level element or br tag. Does not return html
        function trimToFirstEndTag(str, tagRegex) {
            var firstEndTagIndex,
                changedString;

            changedString = str ? str.replace(tagRegex, REPLACEMENT_CHAR).replace(REGEX_HTML_TAG, '') : '';
            firstEndTagIndex = changedString.indexOf(REPLACEMENT_CHAR);
            changedString = changedString.substr(0,  firstEndTagIndex > 0 ? firstEndTagIndex : changedString.length);

            return changedString;
        }

        // Remove any HTML entities ie. &nbsp;
        function sanitizeHTMLEntities(str) {
            var div = angular.element('<div>' + str + '</div>');

            return div.text().replace(REGEX_NBSP, " ");
        }

        /**
         * Trims a string to a given limit. Once the limit is reached. Will trim backwards 
         * to the nearest sentence end, if no sentence end is found will trim to nearest word.
         * @param  {string} str   String to be trimmed
         * @param  {string} limit Length to limit to
         * @return {string}       Trimmed string
         */
        function trimToLimit(str, limit) {
            var indexOfSentenceEnd,
                matches;

            if (str.length > limit){
                // trim to limit
                str = str.substr(0, limit);
                
                // Trim to the nearest sentence end or space
                matches = str.match(REGEX_ENDING_PUNCTUATION);
                if (matches && matches.length){
                    indexOfSentenceEnd = str.lastIndexOf(matches[matches.length-1]);
                    str = str.substr(0, indexOfSentenceEnd + 1); // don't remove punctuation
                } else {
                    str = trimToLastSpace(str);
                }
            }

            return str;
        }

        /**
         * Add a truncation string to a string without going over a limit.
         * Only add the truncation if the string does not end with a full sentence ie .!? characters
         */
        function addTruncationString(str, truncationString, limit) {
            // Add a truncation string if the last character does not signify the end of a sentence
            if(truncationString && _.isString(truncationString) && str.length && !str[str.length-1].match(REGEX_ENDING_PUNCTUATION)){
                // If last character is punctuation
                str = str[str.length-1].match(REGEX_NON_ENDING_PUNCTUATION) ? str.substr(0, str.length-1) : str;
                
                // Trim to the last space if adding the truncation string will put the string over the limit
                if (str.length + truncationString.length > limit) {
                    str = trimToLastSpace(str);

                    // if trimming to the last space was not enough or no space was found, just chop it
                    if(str.length + truncationString.length > limit) {
                        str = str.substr(0, limit - truncationString.length);
                    }
                }
                // Append ellipses
                str += truncationString;
            }

            return str;
        }

        return function(text, limit, truncationString, breakOnBrTag) {
            var changedString;

            changedString = breakOnBrTag ? trimToFirstEndTag(text, REGEX_END_TAG_WITH_BR) : trimToFirstEndTag(text, REGEX_END_TAG);
            changedString = sanitizeHTMLEntities(changedString);
            changedString = trimToLimit(changedString, limit);

            if (truncationString) {
                changedString = addTruncationString(changedString, truncationString, limit);
            }

            return changedString;
        };
    }

    angular.module('origin-components')
        .filter('limitHtml', limitHtml);
}());
