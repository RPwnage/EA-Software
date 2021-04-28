/**
 * extract the javascript vars referenced by the @param tags eg. var TrialTypeEnumeration = {json};
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    var util = require(__dirname + '/util.js')();

    /**
     * Require a source file and attempt to extract constants to help build the
     * option lists for jsdoc referenced types
     *
     * @param {string} sourceCode the source code to analyze
     * @param {Array[Object]} params extracted parameter list
     * @return {Object} a collection containing referenced constants.
     */
    function getTypes(sourceCode, params) {
        var collection = {};

        for (var i = 0; i < params.length; i++) {
            if (params[i].types[0]) {
                var primaryType = params[i].types[0];

                if (util.endsWith(primaryType, 'Enumeration') ||
                    util.endsWith(primaryType, 'Date') ||
                    util.endsWith(primaryType, 'Time')
                ) {
                    collection[primaryType] = extractType(sourceCode, primaryType);
                }
            }
        }

        return collection;
    }

    /**
     * Find referenced enumerations and parse them into a usable javascript object
     *
     * @param {string} sourceCode the source code to analyze
     * @param {string} type the type reference to find eg. *Enumeration
     * @return {object} the parsed enumaration
     */
    function extractType(sourceCode, type) {
        var matchPattern = 'var\\s?' + type + '\\s?=\\s?{([\\s\\S]+?)}';
        var matchVariables = new RegExp(matchPattern, 'g');
        var matches = matchVariables.exec(sourceCode);
        if (matches === null || matches[1] === undefined) {
            throw 'Could not find referenced Object: ' + type + ' in the file analyzed. Please check the spelling and formatting of the enum variable';
        }
        var json = '{' + matches[1] + '}';
        try {
            return JSON.parse(json);
        } catch (err) {
            throw 'Jsdoc referenced objects should be JSON compliant. Please use double quotes to encode strings in jsdoc object declarations. Error: ' + err;
        }
    }

    return {
        getTypes: getTypes
    };
};

