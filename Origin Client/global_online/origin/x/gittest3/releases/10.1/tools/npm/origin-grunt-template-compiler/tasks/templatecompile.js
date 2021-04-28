/**
 * origin-grunt-template-compiler
 *
 * @see README.md for purpose and usage instructions
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';
var chalk = require('chalk');
var minify = require('html-minifier').minify;

module.exports = function(grunt) {
    grunt.registerMultiTask('templatecompile', 'Optimize directive templates', function() {
        var options = this.options();

        //in case of multiple file location, run through all file collections
        this.files.forEach(function(fileList) {
            //take all globbed files found by grunt's file matching tool
            fileList.src.forEach(function(file) {
                try {
                    //get paths from the gruntfile
                    var sourcePath = fileList.cwd + '/' + file;
                    var destinationPath = fileList.dest + '/' + file;
                    var directiveContent = grunt.file.read(sourcePath);

                    //get a list of the template references in the directive file
                    var templateList = directiveContent.match(/templateUrl ?: ?['"](.*)?['"]/gi);
                    if (isCompilableDirective(templateList, directiveContent)) {
                        grunt.log.write(chalk.green("\r\nOptimizing template: " + sourcePath + "\r\n"));
                        var optimizedTemplate = parseTemplate(templateList, directiveContent, options);

                        grunt.log.write(chalk.cyan("Writing template to target location: " + destinationPath + "\r\n"));
                        grunt.file.write(destinationPath, optimizedTemplate);
                    }
                } catch (err) {
                    grunt.log.write(chalk.grey("Skipped: " + file + "\r\n"));
                    return;
                }
            });
        });
    });

    /**
     * @description parse the directive template for template urls and replace/minify accordingly
     * @param {string[]} templateList - a string array of matches ["templateUrl: '@path/to/template.html'", "..."]
     * @param {string} directiveContent - the document to be modified
     * @param {object} options - a list of options set in the gruntfile
     * @returns {string} the updated document
     */
    function parseTemplate(templateList, directiveContent, options) {
        templateList.forEach(function(template) {
            //Cut the templated string at the ':', remove whitespace and quotes to get the path reference
            var fileReference = template.split(':')[1].
                                         trim().
                                         replace(/\'/g, '').
                                         replace(/\"/g, '');

            var htmlFile = resolveHtmlPath(fileReference, options);

            //Load and minfy the html template
            grunt.log.write(chalk.cyan('Processing fileReference: ' + fileReference + ' from file: ' + htmlFile + "\r\n"));
            var htmlData = null;
            try {
                htmlData = minify(grunt.file.read(htmlFile), options);
            } catch (err) {
                grunt.log.write(chalk.red("Compilation Failed. The HTML file may be badly formed or missing. Error message: " + err + "\r\n"));
                throw "Compilation Failed. The HTML file may be badly formed or missing. Error message: " + err;
            }

            //replace the templateUrl: @path/to/fileReference.html with the stringified version template: "<html/>"
            directiveContent = directiveContent.replace(template, 'template: ' + JSON.stringify(htmlData));
        });

        return directiveContent;
    }


    /**
     * @description Replace the @path to the one set in options/paths[]
     * @param {string} fileReference - a path like @path_to/html/asset.html
     * @param {object} options - a list of options set in the gruntfile
     * @returns {string} a real source path to the html src/html/asset.html
     */
    function resolveHtmlPath(fileReference, options) {
        for (var path in options.paths) {
            fileReference = fileReference.replace(path, options.paths[path]);
        }

        return fileReference;
    }

    /**
     * @description Test whether this is a valid file to process
     * @param {string[]} templateList - a string array of matches ["templateUrl: '@path/to/template.html'", "..."]
     * @param {string} directiveContent - the document to be modified
     * @return {boolean} true if it's compilable
     */
    function isCompilableDirective(templateList, directiveContent) {
        return (templateList.length > 0 &&
                directiveContent.indexOf('.directive') > 0);
    }
};
