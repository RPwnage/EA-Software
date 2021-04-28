'use strict';

module.exports = function(grunt) {

    var configReader = require('../tools/buildUtils/config-reader.js')(grunt);

    var OPTIONS_LOCATION = './build/options';

    function readOptions(fileName) {
        var options = require(OPTIONS_LOCATION + '/' + fileName);

        return options();
    }

    // Load grunt tasks automatically
    require('load-grunt-tasks')(grunt);

    // Time how long tasks take. Can help when optimizing build times
    require('time-grunt')(grunt);

    // Load configuration builder
    grunt.loadNpmTasks('grunt-ng-constant');

    // Load the pre minifier dependecy injection tool
    grunt.loadNpmTasks('grunt-ng-annotate');

    // load lesslint
    grunt.loadNpmTasks('grunt-lesslint');

    // template compiler
    grunt.loadNpmTasks('origin-grunt-template-compiler');

    // Define the configuration for all the tasks
    grunt.initConfig({

        // Project settings
        yeoman: {
            // configurable paths
            app: require('./bower.json').appPath || 'src',
            dist: 'dist'
        },

        ngconstant: {
            options: {
                dest: '.tmp/configs/components-config.js',
                name: 'origin-components-config',
                wrap: true
            },
            default: configReader.readConfig({
                id: 'COMPONENTSCONFIG',
                filename: '../tools/config/components-config.json'
            })
        },

        watch: readOptions('watch.js'),
        connect: readOptions('connect.js'),
        jshint: readOptions('jshint.js'),
        csscomb: readOptions('csscomb.js'),
        csslint: readOptions('csslint.js'),
        clean: readOptions('clean.js'),
        less: readOptions('less.js'),
        lesslint: readOptions('lesslint.js'),
        htmlmin: readOptions('htmlmin.js'),
        ngAnnotate: readOptions('ng-annotate.js'),
        copy: readOptions('copy.js'),
        cssmin: readOptions('cssmin.js'),
        uglify: readOptions('uglify.js'),
        concat: readOptions('concat.js'),
        karma: readOptions('karma'),
        templatecompile: readOptions('templatecompile')
    });

    grunt.registerTask('serve', function(target) {
        if (target === 'dist') {
            return grunt.task.run(['build', 'connect:dist:keepalive']);
        }

        grunt.task.run([
            'clean:server',
            'connect:livereload',
            'watch'
        ]);
    });

    grunt.registerTask('server', function() {
        grunt.log.warn('The `server` task has been deprecated. Use `grunt serve` to start a server.');
        grunt.task.run(['serve']);
    });

    // need this for the vm as we do not want livereload
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


    // CSS distribution task.
    grunt.registerTask('dist-css', ['less', 'csscomb']);

    grunt.registerTask('test', [
        'clean:server',
        'ngconstant',
        'copy:karma-test',
        'templatecompile',
        'concat:karma-test',
        'karma'
    ]);

    // setup specific task as per Richard's request
    grunt.registerTask('setup-vm', [
        'clean:dist',
        'ngconstant',
        'less',
        'cssmin',
        'concat:dist',
        'copy:dist'
    ]);

    grunt.registerTask('build', [
        'clean:dist',
        'ngconstant',
        'ngAnnotate',
        'dist-css',
        'csslint',
        'cssmin',
        'concat:dist',
        'copy:dist',
        'uglify',
        'htmlmin'
    ]);

    grunt.registerTask('default', [
        'newer:jshint',
        'build'
    ]);
};
