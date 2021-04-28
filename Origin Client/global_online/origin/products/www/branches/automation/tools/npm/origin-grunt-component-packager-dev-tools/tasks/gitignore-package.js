/**
 * Generate a .gitignore file for a generated project
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function(grunt) {
    grunt.registerMultiTask('package-gitignore', 'Create a gitignore for all files in a target directory', function() {
        var chalk = require('chalk'),
            pathList = [];

        grunt.log.write(chalk.cyan("Packaging .gitignore file for dir\r\n"));

        //In case of multiple file locations, run through all file collections
        this.files.forEach(function(fileList) {
            pathList = pathList.concat(fileList.src);
        });

        grunt.file.write(
            this.files[0].dest + '/.gitignore',
            pathList.filter(pathFilter).join("\r\n")
        );
    });

    /**
     * Pipeline to filter paths allowed in a p4ignore file
     *
     * @param {string} path the path to analyze
     * @param {boolean} pass/fail the path
     */
    function pathFilter(path) {
        return (path &&
            path.length > 0 &&
            path.indexOf('.') > -1
        );
    }
};
