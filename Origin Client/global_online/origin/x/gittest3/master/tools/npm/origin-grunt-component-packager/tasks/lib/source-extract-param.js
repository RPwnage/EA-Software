/**
 * Parse the @param field
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    /**
     * Parse the parameters into a struct
     * matches @param {:type} :name :description
     *
     * @param {string} entry the string to analyze
     * @return {object} an object containing the type, name and description
     */
    function mapParams(entry) {
        if (entry.indexOf('@param ') > -1) {
            return {
                types: parseParamType(entry),
                name: parseParamName(entry),
                description: parseParamDescription(entry)
            };
        }
    }

    /**
     * Parse the parameter field for a list of types
     *
     * @param {string} entry the @param line to analyze
     * @return {array} a list of types
     */
    function parseParamType(entry) {
        var matches = entry.match(/@param\s?{?\(?([\s\S]+?)\)?}/);
        if (matches && matches[1]) {
            return matches[1].split("|").map(sanitizeTypes);
        }

        return [];
    }

    /**
     * Sanitize types for use by the component importer
     *
     * @param {string} type the unwashed text input
     * @return {string} the sanitized type name
     */
    function sanitizeTypes(type) {
        return type.
            replace('=', '').
            replace('[]', '').
            trim();
    }

    /**
     * Parse the parameter field for the name
     *
     * @param {string} entry the @param line to analyze
     * @return {string} the sanitized param name
     */
    function parseParamName(entry) {
        var matches = entry.match(/@param[\s\S]+?}\s?(.*)/),
            name = '';

        if (matches && matches[1]) {
            if (matches[1].charAt(0) === '[') {
                var optionalMatches = matches[1].match(/\[(.*?)[\s=\]]/);
                if (optionalMatches && optionalMatches[1]) {
                    name = optionalMatches[1];
                } else {
                    throw 'Optional block is malformed';
                }
            } else {
                var requiredMatches = matches[1].split(' ');
                if (requiredMatches && requiredMatches[0]) {
                    name = requiredMatches[0];
                } else {
                    throw 'Could not find a name for the param';
                }
            }
        }

        if (name.indexOf('.') > -1) {
            throw 'Parameter properties eg. employees.name not supported in directive contexts. please review the documentation for this file.';
        }

        return name.
            trim().
            toLowerCase();
    }

    /**
     * Parse the parameter field for the description
     *
     * @param {string} entry the @param line to analyze
     * @return {string} the description
     */
    function parseParamDescription(entry) {
        var matches = entry.match(/@param[\s\S]+?}\s?(.*)/),
            description = '';

        if (matches && matches[1]) {
            if (matches[1].charAt(0) === '[') {
                var optionalNameMatches = matches[1].match(/]\s(.*)/);
                if (optionalNameMatches && optionalNameMatches[1]) {
                    description = optionalNameMatches[1];
                } else {
                    throw 'No description found';
                }
            } else {
                description = matches[1].substr(matches[1].indexOf(' ') + 1);
            }
        }

        return description.
            replace('-', '').
            trim();
    }

    /**
     * Validate the parameters
     *
     * @param {object|undefined} entry the entry to analyze
     * @return {Boolean} true if defined
     */
    function validateParams(entry) {
        if (entry && (
            entry.types.length === 0 ||
            entry.name.length === 0 ||
            entry.description.length === 0
        )) {
            throw 'Parameters incompletely documented. Please review that the directive documentation is properly formatted for this file.';
        }

        for (var i = 0; i < entry.types.length; i++) {
            if (entry.types[i] === '*') {
                throw 'Wildcard types cannot be interpreted. Please review that the directive documentation is properly formatted';
            }
            if (entry.types[i].indexOf('...') > -1) {
                throw 'Repeated parameters cannot be interpreted. Please review that the directive documentation is properly formatted';
            }
        }

        return true;
    }
    return {
        mapParams: mapParams,
        validateParams: validateParams
    };
};
