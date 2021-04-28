/**
 * Parse source code and build a source code mapping model for the packager
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

var docblockParser = require(__dirname + '/source-extract-docblock.js')(),
    infoParser = require(__dirname + '/source-extract-info.js')(),
    paramParser = require(__dirname + '/source-extract-param.js')(),
    typeParser = require(__dirname + '/source-extract-type.js')();

module.exports = function() {
    /**
     * Build a packager model
     *
     * @param {string} sourceCode the source code file to analyze
     * @return {object} the packager model
     */
    function extract(sourceCode) {
        var rawBlock = docblockParser.getRawBlock(sourceCode);
        var info = rawBlock.
            map(infoParser.mapInfo).
            filter(filterUndefined);
        var params = rawBlock.
            map(paramParser.mapParams).
            filter(filterUndefined).
            filter(paramParser.validateParams);
        var types = typeParser.getTypes(sourceCode, params);

        return {
            info: info[0],
            params: params,
            types: types
        };
    }

    /**
     * Remove undefined fields found in the mapping process
     *
     * @param {object|undefined} entry the entry to analyze
     * @return {Boolean} true if defined
     */
    function filterUndefined(entry) {
        return (entry !== undefined);
    }

    return {
        extract: extract
    };
};

