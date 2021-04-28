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
               replace(/%directive-images%/gm, getTabsPartial(parameters, 'ImageUrl', __dirname + '/../templates/image-tabs.jsp.partial.tpl')).
               replace(/%directive-videos%/gm, getTabsPartial(parameters, 'Video', __dirname + '/../templates/video-tabs.jsp.partial.tpl')).
               replace(/%directive-offers%/gm, getTabsPartial(parameters, 'OfferId', __dirname + '/../templates/offer-tabs.jsp.partial.tpl')).
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
        var paramName = param.name.replace(/^\*/,"");

        if (primaryType === "LocalizedString" || primaryType === "LocalizedText" ||
            primaryType === "LocalizedTemplateText" || primaryType === "LocalizedTemplateString") {
            return '"i18n/' + paramName + '"';
        } else {
            return '"' + paramName + '"';
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
        var paramName = param.name.replace(/^\*/,"");

        if (primaryType === "ImageUrl") {
            return paramName + ': Double Click to View Image' + "\n";
        } else if (primaryType === "DateTime") {
            return paramName + ': Double Click to View The Timestamp' + "\n";
        } else if (primaryType === "Video") {
            return paramName + ': Double Click to View The Video' + "\n";
        } else if (primaryType === "OfferId") {
            return paramName + ': Double Click to View The Offer' + "\n";
        } else if (primaryType === "OcdPath") {
            return paramName + ': <%=properties.get("' + paramName + '")%>' + "\n";
        } else if (primaryType === "LocalizedString" || primaryType === "LocalizedText" ||
            primaryType === "LocalizedTemplateText" || primaryType === "LocalizedTemplateString") {
            return paramName + ': <%=properties.get("i18n/' + paramName + '")%>' + "\n";
        } else {
            return paramName + ': <%=properties.get("' + paramName + '")%>' + "\n";
        }
    }

    /**
     * Create a tab property list
     *
     * @param {Object} parameters the parameters object
     * @param {string} primaryType the type of object to add to the tab group eg Video
     * @param {string} templatePath the template partial file to read eg. templates/video-tabs.jsp.partial.tpl
     * @return {string} the partial to include in the tabs template
     */
    function getTabsPartial(parameters, primaryType, templatePath) {
        var tabs = [];
        if (parameters && parameters.length > 0) {
            parameters.forEach(function(param) {
                var testPrimaryType = param.types[0];

                if (testPrimaryType === primaryType) {
                    tabs.push('"' + param.name + '"');
                }
            });
        }

        if (tabs.length > 0) {
            var template = fs.readFileSync(path.resolve(templatePath)).toString();
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
