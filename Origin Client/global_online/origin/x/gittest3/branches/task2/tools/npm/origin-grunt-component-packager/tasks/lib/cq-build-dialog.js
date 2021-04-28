/**
 * Create the dialog file from the jsdoc name, parameters and enums
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function(encoding) {
    var builder = require('xmlbuilder'),
        builderOptions = { version: '1.0', encoding: encoding, standalone: true },
        serializerOptions = { pretty: true, indent: '  ', newline: '\r\n' },
        util = require(__dirname + '/util.js')(),
        templates = require(__dirname + '/dialog-xml-templates.js')();

    /**
     * @description produces CQ5 component translation page as XML string
     * @param {Object} source the source code model
     * @return {string} XML document
     */
    function build(source) {
        var dialog = templates.dialogContainer(source.info.directiveName);
        var node = dialog['jcr:root']['items']['items'];

        source.params.forEach(function(parameter) {
            var primaryType = parameter.types[0];

            if (primaryType) {
                if (primaryType === "ImageUrl") {
                    node[parameter.name] = templates.imageTab(parameter);
                } else if (primaryType === "LocalizedString") {
                    node['general']['items'][parameter.name] = templates.localizedString(parameter);
                } else if (primaryType === "LocalizedText") {
                    node['general']['items'][parameter.name] = templates.localizedText(parameter);
                } else if (primaryType === "Url") {
                    node['general']['items'][parameter.name] = templates.url(parameter);
                } else if (primaryType.toLowerCase() === 'number') {
                    node['general']['items'][parameter.name] = templates.number(parameter);
                } else if (util.endsWith(primaryType, 'Enumeration')) {
                    node['general']['items'][parameter.name] = templates.enumeration(parameter, source.types[primaryType]);
                } else if (util.endsWith(primaryType, 'Date')) {
                    node['general']['items'][parameter.name] = templates.date(parameter, source.types[primaryType]);
                } else if (util.endsWith(primaryType, 'Time')) {
                    node['general']['items'][parameter.name] = templates.time(parameter, source.types[primaryType]);
                } else if (util.endsWith(primaryType, 'DateTime')) {
                    node['general']['items'][parameter.name] = templates.date(parameter, source.types[primaryType]);
                } else {
                    node['general']['items'][parameter.name] = templates.string(parameter);
                }
            } else {
                throw 'No primary type found. The directive declaration may be malformed. Check the type field for ' + source.info.directiveName;
            }
        });

        node['contenttarget'] = templates.geoTargetingTab();
        node['contentreferencetarget'] = templates.contentReferenceTab();
        if (isOcdAware(source.params)) {
            node['offertarget'] = templates.offerTargetingTab();
        }

        return builder.create(dialog, builderOptions).end(serializerOptions);
    }

    /**
     * Determine if the directive is Origin Content Database (OCD) aware
     *
     * @param {Array} params the list of params extracted from the directive docs
     * @return true if any parameter has a subtype of OCD
     */
    function isOcdAware(params) {
        for (var i = 0; i < params.length; i++) {
            if (params[i].types.indexOf('OCD') > -1) {
                return true;
            }
        }

        return false;
    }

    return {
        build: build
    };
};
