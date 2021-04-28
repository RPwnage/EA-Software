/**
 * Tools to parse the @name jsdoc field
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    /**
     * Parse the directive information a struct
     * matches @name :directiveGroup[:]:normalizedDirectiveName
     *
     * @param {string} entry the string to analyze
     * @return {object} an object containing normalizedDirectiveName, elementName
     */
    function mapInfo(entry) {
        var matches = entry.match(/@name\s?([a-zA-Z0-9_-]+.[a-zA-Z0-9_-]+):([a-zA-Z0-9_-]+)/);
        if (matches) {
            var normalizedDirectiveName = null,
                directiveName = null;

            if (matches[2]) {
                normalizedDirectiveName = matches[2];
                directiveName = matches[2].replace(/([A-Z])/g, '-$1').toLowerCase();

                if (directiveName[0] === '-') {
                    throw 'This directive is not named properly. it should match the format: origin-components.directives:originSectionDirective (CASE SENSITIVE)';
                }
            } else {
                throw 'This directive does not have an @name attribute or is malformed';
            }

            return {
                normalizedDirectiveName: normalizedDirectiveName,
                directiveName: directiveName
            };
        }
    }

    return {
        mapInfo:mapInfo
    };
};
