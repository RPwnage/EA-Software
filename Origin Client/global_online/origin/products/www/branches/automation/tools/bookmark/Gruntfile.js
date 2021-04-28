'use strict';

module.exports = function(grunt) {

    var OPTIONS_LOCATION = './build/options';

    function readOptions(fileName) {
        var options = require(OPTIONS_LOCATION + '/' + fileName);

        return options();
    }

    // Load grunt tasks automatically
    require('load-grunt-tasks')(grunt);

    // Define the configuration for all the tasks
    grunt.initConfig({
        uglify: readOptions('uglify.js')
    });

    grunt.registerTask('minify', [
        'uglify'
    ]);
};
