/**
 * Build a design view template - this will set up the sidekick UI in CQ5
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    var builder, builderOptions, serializerOptions, design;

    /**
     * Initialize the design builder
     *
     * @param {string} workspaceName the friendly name of the workspace (eg. OriginX Defaults)
     * @param {string} encoding the encoding for XML serialization
     */
    function init(workspaceName, encoding) {
        builder = require('xmlbuilder');
        builderOptions = { version: '1.0', encoding: encoding, standalone: true };
        serializerOptions = { pretty: true, indent: '  ', newline: '\r\n' };
        design = {
            "jcr:root": {
                "@xmlns:sling": "http://sling.apache.org/jcr/sling/1.0",
                "@xmlns:cq": "http://www.day.com/jcr/cq/1.0",
                "@xmlns:jcr": "http://www.jcp.org/jcr/1.0",
                "@xmlns:nt": "http://www.jcp.org/jcr/nt/1.0",
                "@jcr:primaryType": "cq:Page",
                "jcr:content": {
                    "@cq:template": "/libs/wcm/core/templates/designpage",
                    "@jcr:primaryType": "cq:PageContent",
                    "@jcr:title": workspaceName,
                    "@sling:resourceType": "wcm/core/components/designer"
                }
            }
        };
    }

    /**
     * Add a component to the design model
     *
     * @param {string} pageType the page type eg. [/apps/originx/pages/][my-page] -- "my-page" is the pageType value
     * @param {array} directivePaths the path to the allowed component for this page eg. /apps/originx/directives/my-directive
     */
    function addComponent(pageType, directivePaths) {
        design['jcr:root']['jcr:content'][pageType] = {
            "@jcr:primaryType": "nt:unstructured",
            "items": {
                "@jcr:primaryType": "nt:unstructured",
                "@sling:resourceType": "foundation/components/parsys",
                "@components": formatDirectivePaths(directivePaths),
                "section": {
                    "@jcr:primaryType": "nt:unstructured"
                }
            }
        };
    }

    /**
     * Format the directive path list for the components attribute in CQ
     *
     * @param {array} directivePaths list of directive paths ["/path/to/1", "/path/to/2"]
     * @return {string} a cq compatible string serialized property list [/path/to/1,/path/to/2] or empty string
     */
    function formatDirectivePaths(directivePaths) {
        if (directivePaths && directivePaths.length > 0) {
            return "[" + directivePaths.join(',') + "]";
        } else {
            return "";
        }
    }

    /**
     * Export Design template as XML
     *
     * @return {string} XML
     */
    function asXml() {
        return builder.create(design, builderOptions).end(serializerOptions);
    }

    return {
        init: init,
        addComponent: addComponent,
        asXml: asXml
    };
};
