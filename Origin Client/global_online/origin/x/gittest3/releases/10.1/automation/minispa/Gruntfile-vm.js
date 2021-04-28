/**
 * This gruntfile is for use with headless tasks used by the Virtual Machine
 * to speed up local development. If you make changes to this file, please
 * test your changes by running ./setup in the tools folder.
 */
'use strict';

module.exports = function(grunt) {
    require('load-grunt-tasks')(grunt);

    var appConfig = {
        app: 'src',
        dist: 'dist'
    };

    grunt.initConfig({
        yeoman: appConfig,

        watch: {
            vm: {
                files: ['<%= yeoman.app %>/**'],
                tasks: ['default']
            }
        },

        clean: {
            dist: {
                files: [{
                    dot: true,
                    src: [
                        '.tmp',
                        '<%= yeoman.dist %>/{,*/}*',
                        '!<%= yeoman.dist %>/.git*',
                        '!<%= yeoman.dist %>/bower_components/**'
                    ]
                }]
            },
            server: '.tmp'
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
                        'minispa_data/{,*/}*.*',
                        'minispa_directives/{,*/}*.*',
                        'minispa_js/{,*/}*.js',
                        'minispa_style/{,*/}*.css'
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

    grunt.registerTask('watch-vm', 'Watcher daemon specific to the virtual machine (headless).', function(target) {
       grunt.task.run(['watch:vm']);
    });

    grunt.registerTask('default', [
        'clean:dist',
        'copy:dist',
        'wiredep'
    ]);
};