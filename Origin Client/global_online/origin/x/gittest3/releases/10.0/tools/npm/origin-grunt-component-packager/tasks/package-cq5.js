/**
 * Extract ngdoc interface from codebase, generate CQ/AEM dialog and JSP files under apps
 * and output a zip file for package distribution
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

module.exports = function(grunt) {
    grunt.registerMultiTask('package-cq5', 'Create a CQ/AEM package from ngdocs', function() {
        var chalk = require('chalk'),
            path = require('path'),
            options = this.options(),
            sourceExtractor = require(__dirname + '/lib/source-extractor.js')(),
            util = require(__dirname + '/lib/util.js')(),
            directivePackager = require(__dirname + '/package-directives.js')(grunt, options),
            defaultsPackager = require(__dirname + '/package-default-workspace.js')(grunt, options),
            devPackager = require(__dirname + '/package-dev-workspace.js')(grunt, options),
            OcdPackager = require(__dirname + '/package-ocd-info')(grunt, options);

        defaultsPackager.init("OriginX Defaults", options.encoding);
        devPackager.init("OriginX Dev", options.encoding);

        grunt.log.write(chalk.cyan("Packaging Directives for CQ/AEM\r\n"));
        //in case of multiple file location, run through all file collections
        this.files.forEach(function(fileList) {
            //take all globbed files found by grunt's file matching tool
            fileList.src.forEach(function(file) {
                grunt.log.write(chalk.cyan(file + "\r\n"));
                try {
                    //component configuration
                    var sourceCode = grunt.file.read(path.resolve(fileList.cwd + '/' + file));                    // eg. source code as a string
                    var source = sourceExtractor.extract(sourceCode);                                             // eg. extracted source model
                    var directiveGroup = util.getDirectiveGroupName(file);                                        // eg. game
                    var crxNodeName =  util.getCrxNodeName(source.info.directiveName, directiveGroup, 'origin');  // eg. recommended-next-action-trial

                    // process the source into CQ5 compatible files
                    directivePackager.write(source, directiveGroup, crxNodeName);
                    if (source.params.length > 0) {
                        defaultsPackager.writeApplicationData(source, directiveGroup, crxNodeName);
                    }
                    devPackager.addComponentPath(options.componentsDirectivesCrxPath + '/' + directiveGroup + '/' + crxNodeName);
                    OcdPackager.extract(source);
                } catch (e) {
                    grunt.log.write(chalk.red(e + "\r\n"));
                }
            });
        });

        directivePackager.write(OcdPackager.getSource(), 'ocd', 'ocd');
        defaultsPackager.writeDesignData();
        devPackager.writeDesignData();
    });


};
