/**
 * Generate a .p4ignore file for a generated project
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function(grunt) {
    grunt.registerMultiTask('package-p4ignore', 'Create a P4 Ignore for all files in a target directory', function() {
        var chalk = require('chalk'),
            pathList = [],
            ignoreCollection = {};

        grunt.log.write(chalk.cyan("Packaging .p4ignore file for dir\r\n"));

        //In case of multiple file locations, run through all file collections
        this.files.forEach(function(fileList) {
            pathList = pathList.concat(fileList.src);
        });

        //We'll need to make a .p4ignore for every subdirectory in oder to support both p4v and eclipse
        pathList.filter(pathFilter).forEach(function(file) {
            var pieces = file.split('/');
            var filename = pieces.pop();
            var dir = pieces.join('/');

            if (ignoreCollection[dir] && ignoreCollection[dir].constructor === Array) {
                ignoreCollection[dir].push(filename);
            } else {
                ignoreCollection[dir] = ['.p4ignore', filename];
            }
        });

        //Write each p4ignore file in the collection
        for (var path in ignoreCollection) {
            grunt.file.write(this.files[0].dest + '/' + path + '/.p4ignore', ignoreCollection[path].join('\n'));
        }
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
