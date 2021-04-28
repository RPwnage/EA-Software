/**
 * Create the defaults page content
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function() {
    var builder, builderOptions, serializerOptions;

    /**
     * intialize the builder
     *
     * @param encoding the encoding to use for XML serialization
     */
    function init(encoding) {
        builder = require('xmlbuilder');
        builderOptions = { version: '1.0', encoding: encoding, standalone: true };
        serializerOptions = { pretty: true, indent: '  ', newline: '\r\n' };
    }

    /**
     * build the .content.xml content for the default page
     *
     * @param {string} pageTitle - The page name
     * @return {string} XML
     */
    function buildPageContentXml(pageTitle) {
        var contentXml = {
            "jcr:root": {
                "@cq:noDecoration": "{Boolean}true",
                "@xmlns:sling": "http://sling.apache.org/jcr/sling/1.0",
                "@xmlns:cq": "http://www.day.com/jcr/cq/1.0",
                "@xmlns:jcr": "http://www.jcp.org/jcr/1.0",
                "@jcr:primaryType": "cq:Component",
                "@jcr:title": pageTitle,
                "@sling:resourceSuperType": "originx/components/pages/site/defaults"
            }
        };

        return builder.create(contentXml, builderOptions).end(serializerOptions);
    }

    /**
     * Build the .content.xml content for the template
     *
     * @param {string} pageTitle - The page name
     * @param {string} defaultContentWorkspace - the content workspace where defaults will be merchandised eg. /content/web-defaults
     * @param {string} pageLocation - the location of the page component that will render the merchandising UI eg. /apps/originx-defaults/components/pages/my-page
     * @return {string} XML
     */
    function buildTemplateContentXml(pageTitle, defaultContentWorkspace, pageLocation) {
        var contentXml = {
            "jcr:root": {
                "@xmlns:sling": "http://sling.apache.org/jcr/sling/1.0",
                "@xmlns:cq": "http://www.day.com/jcr/cq/1.0",
                "@xmlns:jcr": "http://www.jcp.org/jcr/1.0",
                "@xmlns:nt": "http://www.jcp.org/jcr/nt/1.0",
                "@jcr:primaryType": "cq:Template",
                "@jcr:title": pageTitle,
                "@allowedPaths": formatAllowedPaths(defaultContentWorkspace),
                "@sling:resourceSuperType": "originx/components/pages/page",
                "@ranking": "{Long}1",
                "jcr:content": {
                    "@jcr:primaryType": "cq:PageContent",
                    "@sling:resourceType": formatPageLocation(pageLocation)
                }
            }
        };

        return builder.create(contentXml, builderOptions).end(serializerOptions);
    }

    /**
     * Build the .content.xml for the user facing content Page
     *
     * @param {string} pageTitle - The page name
     * @param {string} templatePath- the path to the template eg. /apps/originx-defaults/templates/my-template
     * @param {string} pageLocation - the location of the page component that will render the merchandising UI eg. /apps/originx-defaults/components/pages/my-page
     * @return {string} XML
     */
    function buildWebContentXml(pageTitle, templatePath, pageLocation) {
        var contentXml = {
            "jcr:root": {
                "@xmlns:sling": "http://sling.apache.org/jcr/sling/1.0",
                "@xmlns:cq": "http://www.day.com/jcr/cq/1.0",
                "@xmlns:jcr": "http://www.jcp.org/jcr/1.0",
                "@xmlns:nt": "http://www.jcp.org/jcr/nt/1.0",
                "@xmlns:mix": "http://www.jcp.org/jcr/mix/1.0",
                "@jcr:primaryType": "cq:Page",
                "jcr:content": {
                    "@jcr:primaryType": "cq:PageContent",
                    "@cq:template": templatePath,
                    "@jcr:title": pageTitle,
                    "@sling:resourceType": formatPageLocation(pageLocation)
                }
            }
        };

        return builder.create(contentXml, builderOptions).end(serializerOptions);
    }

    /**
     * Build the .content.xml for the directive group
     *
     * @param {string} directiveGroup the directive group name
     * @return {string} XML
     */
    function buildWebGroupXml(directiveGroup) {
        var contentXml = {
            "jcr:root": {
                "@xmlns:sling": "http://sling.apache.org/jcr/sling/1.0",
                "@xmlns:cq": "http://www.day.com/jcr/cq/1.0",
                "@xmlns:jcr": "http://www.jcp.org/jcr/1.0",
                "@xmlns:nt": "http://www.jcp.org/jcr/nt/1.0",
                "@jcr:primaryType": "cq:Page",
                "jcr:content": {
                    "@cq:template": "/apps/originx/templates/site/defaultsgroup",
                    "@jcr:primaryType": "cq:PageContent",
                    "@jcr:title": directiveGroup,
                    "@sling:resourceType": "originx/components/pages/site/defaultsgroup"
                }
            }
        };

        return builder.create(contentXml, builderOptions).end(serializerOptions);
    }

    /**
     * Given a desired content path workspace for defaults, generate an allowed paths string
     *
     * @param {string} defaultContentWorkspace the default content path eg. /content/web-defaults
     * @return {string} the formatted property eg. [/content/web-defaults(/.*)?]
     */
    function formatAllowedPaths(defaultContentWorkspace) {
        return '[' + defaultContentWorkspace + '(/.*)?]';
    }

    /**
     * Given a desired content path workspace for defaults, generate an allowed paths string
     *
     * @param {string} pageLocation the page location to format eg. /apps/originx-defaults/components/pages/my-page
     * @return {string} the formatted property eg. originx-defaults/components/pages/my-page
     */
    function formatPageLocation(pageLocation) {
        return pageLocation.replace('/apps/', '');
    }

    return {
        init: init,
        buildPageContentXml: buildPageContentXml,
        buildTemplateContentXml: buildTemplateContentXml,
        buildWebContentXml: buildWebContentXml,
        buildWebGroupXml: buildWebGroupXml
    };
};
