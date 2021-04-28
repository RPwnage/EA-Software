/**
 * origin-grunt-font-compiler
 *
 * @see README.md for purpose and usage instructions
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';
var chalk = require('chalk');
var pathinfo = require('path');
var fs = require('fs');

module.exports = function(grunt) {
    grunt.registerMultiTask('fontcompile', 'Embed Font files as a LESS dictionary before compilation', function() {
        grunt.log.write(chalk.cyan("Optimizing LESS file to include fonts\r\n"));
        // get path to fonts.less
        //in case of multiple file location, run through all file collections
        this.files.forEach(function(fileList) {
            //take all globbed files found by grunt's file matching tool
            fileList.src.forEach(function(file) {
                try {
                    //get paths from the gruntfile
                    var sourcePath = fileList.cwd + '/' + file;
                    var fontBasePath = pathinfo.dirname(sourcePath);
                    var destinationPath = fileList.dest + '/' + file;
                    var lessContent = grunt.file.read(sourcePath);

                    //get a list of font file references in the directive file
                    var templateList = lessContent.match(/^\@font-file-(.*)?['"]/gm);
                    if (templateList !== null && templateList.length > 0) {
                        grunt.log.write(chalk.green("\r\nOptimizing template: " + sourcePath + "\r\n"));
                        var optimizedTemplate = parseTemplate(templateList, lessContent, fontBasePath);

                        grunt.log.write(chalk.cyan("Writing template to target location: " + destinationPath + "\r\n"));
                        grunt.file.write(destinationPath, optimizedTemplate);
                    }
                } catch (err) {
                    grunt.log.write(chalk.red("An error occured during processing: " + err + "\r\n"));
                    return;
                }
            });
        });
    });

    /**
     * @description parse the less file for valid font files and base64 encode them into a string
     * @param {string[]} templateList - a string array of matches ['@font-file-origin-glyph-icons: "../fonts/origin.woff"', '...']
     * @param {string} lessContent - the document to be modified
     * @param {string} fontBasePath - the base path for fonts
     * @returns {string} the updated document
     */
    function parseTemplate(templateList, lessContent, fontBasePath) {
        templateList.forEach(function(template) {
            //Cut the templated string at the ':', remove whitespace and quotes to get the path reference
            var pieces = template.split(':');
            var lessVarName = pieces[0].trim();
            var fileReference = pieces[1].
                                         trim().
                                         replace(/\'/g, '').
                                         replace(/\"/g, '');
            var fontFile = pathinfo.normalize(fontBasePath + '/' + fileReference);
            var fontContent = null;
            var serializedFont = null;
            var mimeType = null;

            grunt.log.write(chalk.cyan('Processing fileReference from file: ' + fontFile  + "\r\n"));
            try {
                fontContent = fs.readFileSync(fontFile);
                serializedFont = new Buffer(fontContent, 'utf8').toString('base64');
            } catch (err) {
                grunt.log.write(chalk.red("Compilation Failed. The font file could not be buffered. Error message: " + err + "\r\n"));
                throw "Compilation Failed. The font file could not be buffered. Error message: " + err;
            }

            //replace the less path variable with the base 64 encoded string
            if (fontFile.substr(-4) === 'woff') {
                mimeType = 'application/x-font-woff';
            } else if (fontFile.substr(-3) === 'ttf') {
                mimeType = 'font/ttf';
            }

             lessContent = lessContent.replace(template, lessVarName + ': "data:' + mimeType + ';charset=utf-8;base64,' + serializedFont + '"');
        });

        return lessContent;
    }
};
