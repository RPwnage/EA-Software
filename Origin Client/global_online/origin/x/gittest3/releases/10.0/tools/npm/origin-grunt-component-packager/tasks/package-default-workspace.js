/**
 * Create the components needed to populate the defaults upon import
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function(grunt, options) {
    var path = require('path'),
        defaultsAppsBuilder = require(__dirname + '/lib/cq-build-defaults.js')(),
        defaultDesignBuilder = require(__dirname + '/lib/cq-build-design.js')();

    /**
     * Initialize the workspace
     *
     * @param {string} workspaceName the friendly name for the workspace
     * @param {string} encoding the encoding to use for XML
     */
    function init(workspaceName, encoding) {
        defaultsAppsBuilder.init(encoding);
        defaultDesignBuilder.init(workspaceName, encoding);
    }

    /**
     * Generate an entry for the page in the apps context
     *
     * @param {object} source the source object from lib/source-extractor
     * @param {string} directiveGroup the directive group name eg. home
     * @param {string} crxnodeName the directive node name eg. origin
     */
    function writeApplicationData(source, directiveGroup, crxNodeName) {
        generateApps(source, directiveGroup, crxNodeName);
        generateDesign(source, directiveGroup, crxNodeName);
        generateContent(source, directiveGroup, crxNodeName);
    }

    /**
     * Design data is buffered for each writeApplicationData call this function will write the final result
     */
    function writeDesignData() {
        grunt.file.write(options.packageDestinationPath + '/jcr_root' + options.defaultsDesignCrxPath + '/.content.xml', defaultDesignBuilder.asXml());
    }

    /**
     * Generate an entry for the page in the apps context
     *
     * @param {object} source the source object from lib/source-extractor
     * @param {string} directiveGroup the directive group name eg. home
     * @param {string} crxnodeName the directive node name eg. origin
     */
    function generateApps(source, directiveGroup, crxNodeName) {
        var pageCrxNode = options.defaultsAppsCrxPath + '/components/pages/' + directiveGroup + '/' + crxNodeName;
        var pagesPath = path.normalize(options.packageDestinationPath + '/jcr_root' + pageCrxNode);
        var templatesPath = path.normalize(options.packageDestinationPath + '/jcr_root' + options.defaultsAppsCrxPath + '/templates/' + directiveGroup + '/' + crxNodeName);

        //add the metadata file
        grunt.file.write(pagesPath + '/.content.xml', defaultsAppsBuilder.buildPageContentXml(source.info.directiveName));

        //Template content for defaults
        grunt.file.write(templatesPath + '/.content.xml',
            defaultsAppsBuilder.buildTemplateContentXml(
                source.info.directiveName,
                options.defaultsContentCrxPath,
                pageCrxNode));
    }

   /**
     * Generate an entry for the page in the content context
     *
     * @param {object} source the source object from lib/source-extractor
     * @param {string} directiveGroup the directive group name eg. home
     * @param {string} crxnodeName the directive node name eg. origin
     */
    function generateContent(source, directiveGroup, crxNodeName) {
        var pageCrxNode = options.defaultsAppsCrxPath + '/components/pages/' + directiveGroup + '/' + crxNodeName;
        var templateCrxNode = options.defaultsAppsCrxPath + '/templates/' + directiveGroup + '/' + crxNodeName;
        var webContentPath = path.normalize(options.packageDestinationPath + '/jcr_root' + options.defaultsContentCrxPath + '/' + directiveGroup);

        //add the group page
        grunt.file.write(webContentPath + '/.content.xml', defaultsAppsBuilder.buildWebGroupXml(directiveGroup));

        //add the component page
        grunt.file.write(webContentPath + '/' + source.info.directiveName + '/.content.xml', defaultsAppsBuilder.buildWebContentXml(
            source.info.directiveName,
            templateCrxNode,
            pageCrxNode));
    }

    /**
     * Generate an entry for the page in the apps context
     *
     * @param {object} source the source object from lib/source-extractor
     * @param {string} directiveGroup the directive group name eg. home
     * @param {string} crxnodeName the directive node name eg. origin
     */
    function generateDesign(source, directiveGroup, crxNodeName) {
        var componentReference = options.componentsDirectivesCrxPath + '/' + directiveGroup + '/' + crxNodeName;

        defaultDesignBuilder.addComponent(crxNodeName, [componentReference]);
    }

    return {
        init: init,
        writeApplicationData: writeApplicationData,
        writeDesignData: writeDesignData
    };
};
