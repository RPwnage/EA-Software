/**
 * Helper functions for the packager tools
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    /**
     * ManlyCase a hyphenated-string
     *
     * @param {string} hyphenatedString a string containing hyphens eg. recommended-action-trial
     * @return {string} a manly case representation of the string eg. RecommendedActionTrial
     */
    function toManlyCase(hyphenatedString) {
        return hyphenatedString.
            split('-').
            map(function(name) {
                return name.charAt(0).toUpperCase() + name.slice(1);
            }).
            join('');
    }

    /**
     * Create a friendly crx pathname for use in node names
     *
     * @param {string} directiveName the element name
     * @param {string} directiveGroup the directive group name
     * @param {string} namespace the namespace of the directive eg. "origin"
     * @return {string} the friendly, shortened node name for crx readability
     */
    function getCrxNodeName(directiveName, directiveGroup, namespace) {
        return directiveName.
            toLowerCase().
            replace(namespace.toLowerCase() + '-' + directiveGroup.toLowerCase() + '-', '');
    }

    /**
     * Get the directive group name
     *
     * @param {string} file the relative path to the file eg. home/recommended/action/scripts/trial.js
     * @return {string} the groupname eg. home
     */
    function getDirectiveGroupName(file) {
        return file.split('/')[0].replace('.js', '');
    }

    /**
     * See if a string ends with a particular suffix
     *
     * @param {string} str haystack string
     * @param {string} suffix search for a string ending
     */
    function endsWith(str, suffix) {
        return str.indexOf(suffix, str.length - suffix.length) !== -1;
    }

    return {
        toManlyCase: toManlyCase,
        getCrxNodeName: getCrxNodeName,
        getDirectiveGroupName: getDirectiveGroupName,
        endsWith: endsWith
    };
};

