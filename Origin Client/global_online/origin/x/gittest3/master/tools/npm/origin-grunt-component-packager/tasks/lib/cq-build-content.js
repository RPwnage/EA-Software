/**
 * Create the dialog file from the jsdoc name, parameters and enums
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function(encoding) {
    var builder = require('xmlbuilder'),
        builderOptions = { version: '1.0', encoding: encoding, standalone: true },
        serializerOptions = { pretty: true, indent: '  ', newline: '\r\n' };

    /**
     * build the content.xml file for the directive directory
     *
     * @param {string} directiveTitle - the proposed jcr title name eg. recommendedActionTrial
     * @param {string} componentGroup - the name of the group the directive belongs to eg. Web: Home
     * @param {string} dialogPath - the path reference to this component's default dialog eg. /apps/originx/imported/dialogs/originHomeRecommended
     * @return {string} XML
     */
    function buildDirectiveContentXml(directiveTitle, componentGroup, dialogPath) {
        var contentXml = {
            "jcr:root": {
                "@xmlns:sling": "http://sling.apache.org/jcr/sling/1.0",
                "@xmlns:cq": "http://www.day.com/jcr/cq/1.0",
                "@xmlns:jcr": "http://www.jcp.org/jcr/1.0",
                "@jcr:primaryType": "cq:Component",
                "@jcr:title": directiveTitle,
                "@allowedParents": "[*/parsys]",
                "@dialogPath": dialogPath,
                "@componentGroup": componentGroup,
                "@sling:resourceSuperType": "originx/components/base"
            }
        };

        return builder.create(contentXml, builderOptions).end(serializerOptions);
    }

    return {
        buildDirectiveContentXml: buildDirectiveContentXml
    };
};
