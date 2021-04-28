/**
 * Create the dialog file from the jsdoc name, parameters and enums
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    var fs = require('fs'),
        path = require('path');

    /**
     * Prepare the basic UI JSP template
     *
     * @param {string} sourceCode - the main source jsp template
     * @param {string} directiveName- the directive name
     * @param {Object} parameters - the parameters extracted from jsdoc
     * @return {string} the modified source code
     */
    function buildDefaultUi(sourceCode, directiveName, parameters) {
        return sourceCode.
               replace(/%directive-name%/gm, directiveName).
               replace(/%directive-display-properties%/gm, parameters.map(mapUiProperties).join(""));
    }

    /**
     * Prepare the main/switcher JSP template
     *
     * @param {string} sourceCode - the main source jsp template
     * @param {string} defaultPath - the default jsp UI path
     * @param {string} overridePath - the GEO override UI path
     * @param {Object} parameters - the parameters extracted from jsdoc
     * @param {string} directivename - the name of the directive
     * @return {string} the modified source code
     */
    function buildMain(sourceCode, defaultPath, overridePath, parameters, directiveName) {
        return sourceCode.
               replace(/%overridePath%/g, overridePath).
               replace(/%defaultPath%/g, defaultPath).
               replace(/%directive-images%/gm, getImageBlock(parameters)).
               replace(/%directive-name%/g, directiveName).
               replace(/%directive-properties%/g, parameters.map(mapDirectiveProperties).join(','));
    }

    /**
     * Create a UI property list given a list of parameters extracted from jsdoc
     *
     * @param {Object} param the parameter object
     * @return a JSP representation of the param for display purposes
     */
    function mapDirectiveProperties(param) {
        var primaryType = param.types[0];

        if (primaryType === "LocalizedString" || primaryType === "LocalizedText") {
            return '"i18n/' + param.name + '"';
        } else {
            return '"' + param.name + '"';
        }
    }

    /**
     * Create a UI property list given a list of parameters extracted from jsdoc
     *
     * @param {Object} param the parameter object
     * @return a JSP representation of the param for display purposes
     */
    function mapUiProperties(param) {
        var primaryType = param.types[0];

        if (primaryType === "ImageUrl") {
            return 'Image(s): Double Click to View Images' + "\r\n";
        } else if (primaryType === "LocalizedString" || primaryType === "LocalizedText") {
            return param.name + ': <%=properties.get("i18n/' + param.name + '")%>' + "\r\n";
        } else {
            return param.name + ': <%=properties.get("' + param.name + '")%>' + "\r\n";
        }
    }

    /**
     * Create the image path properties
     *
     * @param {Object} param the directive parameters object
     * @return {string} the image tab definititon block
     */
    function getImageBlock(parameters) {
        var tabs = [];
        if (parameters && parameters.length > 0) {
            parameters.forEach(function(param) {
                var primaryType = param.types[0];

                if (primaryType === "ImageUrl") {
                    tabs.push('"' + param.name + '"');
                }
            });
        }

        if (tabs.length > 0) {
            var template = fs.readFileSync(path.resolve(__dirname + '/../templates/image-tabs.jsp.partial.tpl')).toString();
            return template.replace(/%directive-tabs%/gm, tabs.join(','));
        } else {
            return '';
        }
    }

    return {
        buildDefaultUi: buildDefaultUi,
        buildMain: buildMain
    };
};
