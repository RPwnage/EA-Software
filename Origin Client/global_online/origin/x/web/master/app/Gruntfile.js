// Generated on 2014-09-03 using generator-angular 0.9.5
'use strict';

// # Globbing
// for performance reasons we're only matching one level down:
// 'test/spec/{,*/}*.js'
// use this if you want to recursively match all subfolders:
// 'test/spec/**/*.js'

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

    // Load the pre minifier dependecy injection tool
    grunt.loadNpmTasks('grunt-ng-annotate');

    // Load ngdocs package
    grunt.loadNpmTasks('grunt-ngdocs');

    // Load configuration builder
    grunt.loadNpmTasks('grunt-ng-constant');


    // Configurable paths for the application
    var pathConfig = {
        app: require('./bower.json').appPath || 'src',
        dist: 'dist'
    };

    // Define the configuration for all the tasks
    grunt.initConfig({

        // Project settings
        yeoman: pathConfig,

        // Config module generator
        ngconstant: {
            options: {
                dest: '.tmp/constants/app-loc-dictionary.js',
                name: 'origin-app-loc-dictionary',
                wrap: true
            },
            dictdev: configReader.readConfig({
                id: 'LOCDICTIONARY',
                filename: 'config/dev.dictionary.json'
            }),
            dictprod: configReader.readConfig({
                id: 'LOCDICTIONARY',
                filename: 'config/prod.dictionary.json'
            }),
            config: configReader.readConfig({
                dest: '.tmp/constants/app-config.js',
                name: 'origin-app-config',
                id: 'APPCONFIG',
                filename: '../tools/config/app-config.json'
            })
        },

        // The actual grunt server settings
        connect: {
            options: {
                protocol: 'https',
                port: 443,
                key: grunt.file.read('./ssl/localapp.key'),
                cert: grunt.file.read('./ssl/localapp.crt'),
                ca: grunt.file.read('./ssl/ca.crt'),

                // Change this to '0.0.0.0' to access the server from outside.
                hostname: '0.0.0.0',
                livereload: 34729
            },
            livereloadhttps: {
                key: grunt.file.read('./ssl/localapp.key'),
                cert: grunt.file.read('./ssl/localapp.crt'),
                port: 34729
            },
            livereload: {
                options: {
                    open: true,
                    middleware: function(connect) {
                        return [
                            connect.static('.tmp'),
                            connect().use(
                                '/bower_components',
                                connect.static('./bower_components')
                            ),
                            connect.static(pathConfig.dist)
                        ];
                    }
                }
            },
            test: {
                options: {
                    port: 9001,
                    middleware: function(connect) {
                        return [
                            connect.static('.tmp'),
                            connect.static('test'),
                            connect().use(
                                '/bower_components',
                                connect.static('./bower_components')
                            ),
                            connect.static(pathConfig.dist)
                        ];
                    }
                }
            },
            dist: {
                options: {
                    open: true,
                    base: '<%= yeoman.dist %>'
                }
            }
        },

        watch: readOptions('watch.js'),
        jshint: readOptions('jshint.js'),
        jsdoc: readOptions('jsdoc.js'),
        csscomb: readOptions('csscomb.js'),
        csslint: readOptions('csslint.js'),
        clean: readOptions('clean.js'),
        autoprefixer: readOptions('autoprefixer.js'),
        wiredep: readOptions('wiredep.js'),
        less: readOptions('less.js'),
        filerev: readOptions('filerev.js'),
        useminPrepare: readOptions('usemin-prepare.js'),
        usemin: readOptions('usemin.js'),
        uglify: readOptions('uglify.js'),
        htmlmin: readOptions('htmlmin.js'),
        ngAnnotate: readOptions('ng-annotate.js'),
        ngdocs: readOptions('ngdocs.js'),
        cdnify: readOptions('cdnify.js'),
        copy: readOptions('copy.js'),
        concurrent: readOptions('concurrent.js'),
        concat: readOptions('concat.js'),
        karma: readOptions('karma.js')
    });


    grunt.registerTask('serve', 'Compile then start a connect web server', function(target) {
        if (target === 'dist') {
            return grunt.task.run(['build', 'connect:dist:keepalive']);
        }

        if (target === 'nobuild') {
            return grunt.task.run([
                'clean:server',
                'connect:livereload',
                'watch'
            ]);
        }

        grunt.task.run([
            'dev',
            'clean:server',
            'connect:livereload',
            'watch'
        ]);
    });

    grunt.registerTask('server', 'DEPRECATED TASK. Use the "serve" task instead', function(target) {
        grunt.log.warn('The `server` task has been deprecated. Use `grunt serve` to start a server.');
        grunt.task.run(['serve:' + target]);
    });


    // need this for the vm as we do not want livereload
    grunt.registerTask('watch:vm', function() {
        var config = {
            vm: {
                files: [
                    '<%= yeoman.app %>/**',
                    'test/fixtures/components.dict.json'
                ],
                tasks: ['setup-vm']
            }
        };
        grunt.config('watch', config);
        grunt.task.run('watch');
    });

    grunt.registerTask('test', [
        'clean:server',
        'ngconstant:dictdev',
        'concurrent:test',
        'autoprefixer',
        'connect:test' //,
        //'karma'
    ]);

    // CSS distribution task.
    grunt.registerTask('dist-css', ['less', 'csscomb']);

    // tasks specific for the setup script as per Richard's request
    grunt.registerTask('setup-vm', [
        'clean:dist',
        'less',
        'ngconstant:dictdev',
        'ngconstant:config',
        'copy:dev',
        'copy:ngconstants',
        'copy:scripts',
        'copy:styles',
        'copy:images',
        'copy:configs'
    ]);

    grunt.registerTask('build', [
        'clean:dist',
        'ngconstant:dictdev', //use dev for now
        'ngconstant:config',
        'copy:configs',
        'dist-css',
        'useminPrepare',
        'concurrent:dist',
        'autoprefixer',
        'concat',
        'ngAnnotate',
        'copy:dist',
        'cssmin',
        'uglify',
        'filerev',
        'usemin',
        'htmlmin'
    ]);

    grunt.registerTask('default', [
        'newer:jshint',
        'dist-css',
        'csslint',
        'test',
        'build'
    ]);

    grunt.registerTask('dev', [
        'clean:dist',
        'ngconstant:dictdev',
        'ngconstant:config',
        'newer:jshint',
        'dist-css',
        'csslint',
        'copy:dev',
        'copy:ngconstants',
        'copy:configs',
        'copy:scripts',
        'copy:styles',
        'copy:images'

    ]);

    grunt.registerTask('docs', [
        'clean:docs',
        'dist-css',
        'concat:docs',
        'copy:dist',
        'ngdocs',
        'copy:docs'
    ]);


};