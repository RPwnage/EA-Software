// Generated on 2014-04-15 using generator-angular 0.7.1
'use strict';

// # Globbing
// for performance reasons we're only matching one level down:
// 'test/spec/{,*/}*.js'
// use this if you want to recursively match all subfolders:
// 'test/spec/**/*.js'

module.exports = function (grunt) {

    var OPTIONS_LOCATION = './build/options';

    function readOptions(fileName) {
        var options = require(OPTIONS_LOCATION + '/' + fileName);

        return options();
    }

    // Load grunt tasks automatically
    require('load-grunt-tasks')(grunt);

    // Time how long tasks take. Can help when optimizing build times
    require('time-grunt')(grunt);

    // Load the pre minifier dependecy injection tool
    grunt.loadNpmTasks('grunt-ng-annotate');

    // Load configuration builder
    grunt.loadNpmTasks('grunt-ng-constant');


    // Define the configuration for all the tasks
    grunt.initConfig({

        // Project settings
        yeoman: {
            // configurable paths
            app: require('./bower.json').appPath || 'src',
            dist: 'dist'
        },
        watch: readOptions('watch.js'),
        jshint: readOptions('jshint.js'),
        clean: readOptions('clean.js'),
        ngAnnotate: readOptions('ng-annotate.js'),
        uglify: readOptions('uglify.js'),
        concat: readOptions('concat.js'),
        karma: readOptions('karma.js'),
    });

    // need this for the vm
    grunt.registerTask('watch:vm', function() {
      var config = {
        vm: {
            files: ['<%= yeoman.app %>/**'],
            tasks: ['setup-vm']
        }
      };
      grunt.config('watch', config);
      grunt.task.run('watch');
    });

    // tasks specific for the setup script as per Richard's request
    grunt.registerTask('setup-vm', [
        'clean:dist',
        'concat:dist',
        'concat:distassets'
    ]);

    grunt.registerTask('build', [
        'clean:dist',
        'concat:dist',
        'concat:distassets',
        'ngAnnotate',
        'uglify',
        'concat:karma-test',
        'karma:unit'
    ]);

    grunt.registerTask('default', [
        'newer:jshint',
        'build'
    ]);

    grunt.registerTask('test', [
        'clean:server',
        'concat:dist',
        'concat:distassets',
        'concat:karma-test',
        'karma:unit'
    ]);
};
