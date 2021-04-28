// Generated on 2014-09-03 using generator-angular 0.9.5
'use strict';

module.exports = function(grunt) {

    // Load grunt tasks automatically
    require('load-grunt-tasks')(grunt);

    // Configurable paths for the application
    var appConfig = {
        app: require('./bower.json').appPath || 'src',
        dist: 'dist'
    };

    var appFiles = [
        'src/*.html',
		'src/*.js',
        'src/minispa_data/{,*/}*.*',
        'src/minispa_directives/{,*/}*.*',
        'src/minispa_js/{,*/}*.js',
        'src/minispa_style/{,*/}*.css',
		'src/workers/{,*/}*.js'
    ];

    // Define the configuration for all the tasks
    grunt.initConfig({

        // Project settings
        yeoman: appConfig,

        watch: {
            bower: {
                files: ['bower.json'],
                tasks: ['wiredep']
            },
            bowerOrigin: {
                files: [
                    'dist/bower_components/origin-components/dist/scripts/*.js',
                    'dist/bower_components/origin-ui-toolkit/dist/*.js',
                    'dist/bower_components/origin-jssdk/dist/*.js',
                    'dist/bower_components/origin-i18n/dist/*.js'
                ],
                options: {
                    livereload: '<%= connect.livereloadhttps %>'
                }
            },
            js: {
                files: appFiles,
                tasks: ['copy:dist:files', 'wiredep'],
                options: {
                    livereload: '<%= connect.livereloadhttps %>'
                }
            }
        },

        copy: {
            dist: {
                files: [{
                    expand: true,
                    dot: true,
                    cwd: '<%= yeoman.app %>',
                    dest: '<%= yeoman.dist %>',
                    src: [
                        '*.html',
                        '*.js',
                        'minispa_data/{,*/}*.*',
                        'minispa_directives/{,*/}*.*',
                        'minispa_js/{,*/}*.js',
                        'minispa_style/{,*/}*.css',
                        'workers/{,*/}*.js'
                    ]
                }]
            },
            configs: {
                files: [{
                    expand: true,
                    dot: true,
                    cwd: '../../tools/config',
                    dest: '<%= yeoman.dist %>/configs',
                    src: [
                        '**/*.json'
                    ]
                }]
            }

        },

        // The actual grunt server settings
        connect: {
            options: {
                protocol: 'https',
                port: 4443,
                key: grunt.file.read('./ssl/minispa.key'),
                cert: grunt.file.read('./ssl/minispa.crt'),

                // Change this to '0.0.0.0' to access the server from outside.
                hostname: '0.0.0.0',
                livereload: 34730
            },
            livereloadhttps: {
                key: grunt.file.read('./ssl/minispa.key'),
                cert: grunt.file.read('./ssl/minispa.crt'),
                port: 34730
            },
            livereload: {
                options: {
                    //open: true,
                    middleware: function(connect) {
                        return [
                            connect().use(
                                '/bower_components',
                                connect.static('./bower_components')
                            ),
                            connect.static(appConfig.dist)
                        ];
                    }
                }
            }
        },

        // Make sure code styles are up to par and there are no obvious mistakes
        jshint: {
            options: {
                jshintrc: '.jshintrc',
                reporter: require('jshint-stylish')
            },
            all: {
                src: [
                    'src/minispa_directives/{,*/}*.js',
                    'src/minispa_js/{,*/}*.js',
                ]
            }
        },

        csslint: {
            options: {
                csslintrc: '.csslintrc'
            },
            src: ['src/minispa_style/mini-spa.css']
        },

        // Empties folders to start fresh
        clean: {
            dist: {
                files: [{
                    dot: true,
                    src: [
                        '<%= yeoman.dist %>/{,*/}*',
                        '!<%= yeoman.dist %>/bower_components/**'
                    ]
                }]
            }
        },

        // Automatically inject Bower components into the app
        wiredep: {
            target: {
                src: [
                    '<%= yeoman.dist %>/*.html'
                ],
                ignorePath: /\.\.\//
            }
        }
    });


    grunt.registerTask('serve', [
        'default',
        'connect:livereload',
        'watch'
    ]);

    grunt.registerTask('default', [
        'clean:dist',
        'jshint',
        'csslint',
        'copy:dist',
        'copy:configs',
        'wiredep'
    ]);
};