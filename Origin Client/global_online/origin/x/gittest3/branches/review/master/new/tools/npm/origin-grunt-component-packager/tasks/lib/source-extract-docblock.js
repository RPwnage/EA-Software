/**
 * Extract a directive block from the source code
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    /**
     * Test matches to ensure exactly one directive is matched otherwise
     * fail the process. In valid cases, return the raw docblock for processing
     *
     * @param {string} sourceCode the string to analyze
     * @return {Array} line by line of the found docblock
     */
    function getRawBlock(sourceCode) {
        var matchDocblock = /\/*\ \@ngdoc\ directive([\s\S]+?)\*\//gm;
        var matches = sourceCode.match(matchDocblock);

        if (matches === null) {
            throw 'No documentation found';
        }
        if (matches.length !== 1) {
            throw 'Multiple matching ngdoc directive documentation blocks found. This file may require refactoring before continuing.';
        }

        return matches[0].split(/[\n\u0085\u2028\u2029]|\r\n?/g);
    }

    return {
        getRawBlock: getRawBlock
    };
};
