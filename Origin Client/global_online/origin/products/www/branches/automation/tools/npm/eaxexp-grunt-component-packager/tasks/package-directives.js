/**
 * Write the components to imported components folder
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function(grunt, options) {
    var jspBuilder, dialogBuilder, contentBuilder, util, path;

    /**
     * Generate a CQ component structure
     *
     * @param {object} source the source object from lib/source-extractor
     * @param {string} directiveGroup the directive group name eg. home
     * @param {string} crxnodeName the directive node name eg. origin
     */
    function write(source, directiveGroup, crxNodeName) {
        jspBuilder = require(__dirname + '/lib/cq-build-jsp.js')();
        dialogBuilder = require(__dirname + '/lib/cq-build-dialog.js')(options.encoding);
        contentBuilder = require(__dirname + '/lib/cq-build-content.js')(options.encoding);
        util = require(__dirname + '/lib/util.js')();
        path = require('path');

        generateDirectives(source, directiveGroup, crxNodeName);
        generateDialogs(source, directiveGroup, crxNodeName);
    }

    /**
     * Generate the imported Directive fileset
     * @param {object} source the source object from lib/source-extractor
     * @param {string} directiveGroup the directive group name eg. home
     * @param {string} crxnodeName the directive node name eg. origin
     */
    function generateDirectives(source, directiveGroup, crxNodeName) {
        var directiveDestinationPath = path.normalize(options.packageDestinationPath + '/jcr_root' + options.componentsDirectivesCrxPath + '/' + directiveGroup + '/' + crxNodeName);

        // Write the Main JSP Controller
        var mainJspTemplate = grunt.file.read(path.resolve(__dirname + '/templates/main.jsp.tpl'));
        var defaultPath = options.componentsDirectivesCrxPath + '/' + directiveGroup + '/' + crxNodeName + '/' + crxNodeName + '.default.jsp';
        var overridePath = options.componentsDirectivesCrxPath + '/' + directiveGroup + '/' + crxNodeName + '/' + crxNodeName + '.override.jsp';
        var mainJsp = jspBuilder.buildMain(mainJspTemplate, defaultPath, overridePath, source.params, source.info.directiveName);
        grunt.file.write(directiveDestinationPath + '/' + crxNodeName + '.jsp', mainJsp);

        // Write the Default UI
        var defaultJspTemplate = grunt.file.read(path.resolve(__dirname + '/templates/default.jsp.tpl'));
        var defaultJsp = jspBuilder.buildDefaultUi(defaultJspTemplate, source.info.directiveName, source.params);
        grunt.file.write(directiveDestinationPath + '/' + crxNodeName + '.default.jsp', defaultJsp);

        // Write the .content.xml file for the directive
        var directiveContentXml = contentBuilder.buildDirectiveContentXml(
            util.toManlyCase(crxNodeName),
            options.sidekickGroupName + directiveGroup,
            options.componentsDialogsCrxPath + '/' + directiveGroup + '/' + crxNodeName
        );
        grunt.file.write(directiveDestinationPath + '/.content.xml', directiveContentXml);
    }

    /**
     * Generate the dialog fileset
     * @param {object} source the source object from lib/source-extractor
     * @param {string} directiveGroup the directive group name eg. home
     * @param {string} crxnodeName the directive node name eg. origin
     */
    function generateDialogs(source, directiveGroup, crxNodeName) {
        var dialogsDestinationPath = path.normalize(options.packageDestinationPath + '/jcr_root' + options.componentsDialogsCrxPath + '/' + directiveGroup);
        var dialogXml = dialogBuilder.build(source);

        // Create the dialog UI entries
        grunt.file.write(dialogsDestinationPath + '/.content.xml', grunt.file.read(__dirname + '/templates/dialog-content.xml.tpl'));
        grunt.file.write(dialogsDestinationPath + '/' + crxNodeName + '.xml', dialogXml);
    }

    return {
        write: write
    };
};
