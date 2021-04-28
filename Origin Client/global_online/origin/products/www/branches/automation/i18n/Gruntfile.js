// Generated on 2014-04-15 using generator-angular 0.7.1
'use strict';

// # Globbing
// for performance reasons we're only matching one level down:
// 'test/spec/{,*/}*.js'
// use this if you want to recursively match all subfolders:
// 'test/spec/**/*.js'

module.exports = function (grunt) {

    var OPTIONS_LOCATION = './build/options',
        configReader = require('../tools/buildUtils/config-reader.js')(grunt);

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

        // Config module generator
        ngconstant: {
            options: {
                wrap: true
            },
            // this is the dictionary file with the defaults
            // which will be merged with the dictionary file from the
            // cms later on.
            dictionary: configReader.readConfig({
                dest: '.tmp/constants/default-dictionary.js',
                name: 'origin-i18n-default-dictionary',
                id: 'DEFAULT_DICTIONARY',
                filename: '../app/test/fixtures/components.dict.json'
            })
        },

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
        jasmine: readOptions('jasmine.js')
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
        'ngconstant',
        'concat:dist'
    ]);

    grunt.registerTask('build', [
        'clean:dist',
        'ngconstant',
        'concat:dist',
        'ngAnnotate',
        'uglify'
    ]);

    grunt.registerTask('default', [
        'newer:jshint',
        //'test',
        'build'
    ]);

    grunt.registerTask('test', [
        'jasmine:test'
    ]);
};
