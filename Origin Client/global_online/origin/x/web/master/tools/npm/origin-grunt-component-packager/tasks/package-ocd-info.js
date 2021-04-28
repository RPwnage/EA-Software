/**
 * Create a product info merchandising module to share individual attributes across directives
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    var parameterBag = {},
        typesBag = {};

    /**
     * Analyze a source file for shared attributes marked with the ProductInfo type
     *
     * @param {object} source the extracted source model
     */
    function extract(source) {
        source.params.forEach(function(param) {
            var primaryType = param.types[0];

            if (param.types.indexOf('OCD') > -1) {
                parameterBag[param.name] = param;

                if (source.types[primaryType]) {
                    typesBag[primaryType] = source.types[primaryType];
                }
            }
        });
    }

    /**
     * Create a source template
     *
     * @return {object} a fixture source object
     */
    function getSource() {
        return {
            "info": {
                "normalizedDirectiveName": "originOcdOcd",
                "directiveName": "origin-ocd-ocd"
            },
            "params": processParams(parameterBag),
            "types": typesBag
        };
    }

    /**
     * Process the parameter bag into a form suitable for the source template
     *
     * @param {object} parameterBag the parameter bag 'footext': {...},...
     * @return {array} an array of the unique params [{...},...]
     */
    function processParams(parameterBag) {
        return Object.keys(parameterBag).map(function(value) {
            return parameterBag[value];
        });
    }

    return {
        extract: extract,
        getSource: getSource
    };
};
