/**
 * This gruntfile is for use with headless tasks used by the Virtual Machine
 * to speed up local development. If you make changes to this file, please
 * test your changes by running ./setup in the tools folder.
 *
 * @author Richard Hoar <rhoar@ea.com>
 */
'use strict';

module.exports = function (grunt) {
    require('load-grunt-tasks')(grunt, {scope: 'devDependencies'});

    var appConfig = {
        app: '.',
        dist: 'dist',
        docs: 'docs',
        libraryHeader: '/*!\n' +
                        ' * OTK v<%= pkg.version %> (<%= pkg.homepage %>)\n' +
                        ' * Copyright 2011-<%= grunt.template.today("yyyy") %> <%= pkg.author %>\n' +
                        ' * Licensed under <%= pkg.license.type %> (<%= pkg.license.url %>)\n' +
                        ' */\n',
    };

    grunt.initConfig({
        yeoman: appConfig,

        pkg: grunt.file.readJSON('package.json'),

        clean: {
            dist: ['dist', 'docs/dist', 'docs/bower_components'],
            server: '.tmp'
        },

        less: {
            compileCore: {
                options: {
                    strictMath: true,
                    sourceMap: true,
                    outputSourceFiles: true,
                    sourceMapURL: '<%= pkg.name %>.css.map',
                    sourceMapFilename: '<%= yeoman.dist %>/<%= pkg.name %>.css.map'
                },
                files: {
                    '<%= yeoman.dist %>/<%= pkg.name %>.css': 'less/otk.less'
                }
            },
            minify: {
                options: {
                    cleancss: true,
                    report: 'min'
                },
                files: {
                    '<%= yeoman.dist %>/<%= pkg.name %>.min.css': '<%= yeoman.dist %>/<%= pkg.name %>.css'
                }
            }
        },

        usebanner: {
            dist: {
                options: {
                    position: 'top',
                    banner: '<%= banner %>'
                },
                files: {
                    src: [
                        '<%= yeoman.dist %>/<%= pkg.name %>.css',
                        '<%= yeoman.dist %>/<%= pkg.name %>.min.css'
                    ]
                }
            }
        },

        copy: {
            fonts: {
                expand: true,
                src: 'fonts/*',
                dest: '<%= yeoman.dist %>/'
            },
            docs: {
                expand: true,
                cwd: './<%= yeoman.dist %>',
                src: [
                    '*',
                    'fonts/*'
                ],
                dest: 'docs/<%= yeoman.dist %>'
            },
            bower: {
                expand: true,
                src: ['./bower_components/**'],
                dest: 'docs'
            }
        },

        watch: {
            vm: {
                files: [
                    '<%= yeoman.app %>/fonts/**',
                    '<%= yeoman.app %>/less/**',
                ],
                tasks: ['default']
            }
        }
    });

    grunt.registerTask('default', [
        'clean',
        'less',
        'usebanner',
        'copy:fonts',
        'copy:bower',
        'copy:docs'
    ]);

    grunt.registerTask('watch-vm', 'Watcher daemon specific to the virtual machine (headless).', function(target) {
       grunt.task.run(['watch:vm']);
    });
};
