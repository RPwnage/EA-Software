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
                    var templateList = getTemplateList(directiveContent, fileList.cwd);

                    if (isCompilableDirective(templateList, directiveContent)) {
                        grunt.log.write(chalk.green("\r\nOptimizing template: " + sourcePath + "\r\n"));
                        var optimizedTemplate = parseTemplate(templateList, directiveContent, options);

                        grunt.log.write(chalk.cyan("Writing template to target location: " + destinationPath + "\r\n"));
                        grunt.file.write(destinationPath, optimizedTemplate);
                    } else {
                        grunt.log.write(chalk.grey("Skipped: " + file + "\r\n"));
                        return;
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
     * @param {Object[]} templateList - list extracted template objects
     * @param {string} directiveContent - the document to be modified
     * @param {object} options - a list of options set in the gruntfile
     * @returns {string} the updated document
     */
    function parseTemplate(templateList, directiveContent, options) {
        templateList.forEach(function(template) {
            grunt.log.write(chalk.cyan("Processing file: " + template.file + "\r\n"));
            var htmlData = null;
            try {
                htmlData = minify(grunt.file.read(template.file), options);
            } catch (err) {
                grunt.log.write(chalk.red("Compilation Failed. The HTML file may be badly formed or missing. Error message: " + err + "\r\n"));
                throw "Compilation Failed. The HTML file may be badly formed or missing. Error message: " + err;
            }

            //replace the templateUrl: @path/to/fileReference.html with the stringified version template: "<html/>"
            directiveContent = directiveContent.replace(template.text, 'template: ' + JSON.stringify(htmlData));
        });

        return directiveContent;
    }

    /**
     * @description Test whether this is a valid file to process
     * @param {string[]} templateList - a string array of matches ["path/to/template.html", "..."]
     * @param {string} directiveContent - the document to be modified
     * @return {boolean} true if it's compilable
     */
    function isCompilableDirective(templateList, directiveContent) {
        return (templateList.length > 0 &&
                directiveContent.indexOf('.directive') > 0);
    }

    /**
     * @description extract the html template list from the directive
     * @param {string} directiveContent the document to analyze
     * @param {string} templateBasePath the base path to the views
     * @return {Object[]} list of file objects {text: the text to find/replace during processing, file: the filename to use for reading html files}
     */
    function getTemplateList(directiveContent, templateBasePath) {
        var templateList = [];
        var pattern = /templateUrl ?: ?ComponentsConfigFactory.getTemplatePath\(['"](.*)?['"]\)/gi;
        var match = null;
        while ((match = pattern.exec(directiveContent)) !== null) {
            if (match[0] && match[0].length > 0 && match[1] && match[1].length > 0) {
               templateList.push(
                    {
                        'text': match[0],
                        'file': templateBasePath + '/' + match[1]
                    }
                );
            } else {
                grunt.log.write(chalk.red("This directive uses an invalid or malformed templateUrl. Please check the reference for errors.\r\n"));
                throw "This directive uses an invalid or malformed templateUrl. Please check the reference for errors.";
            }
        }

        return templateList;
    }
};
