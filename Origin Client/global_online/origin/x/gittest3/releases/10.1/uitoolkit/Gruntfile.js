/*!
 * Origin Toolkits's Gruntfile
 * Copyright 2014 Electronic Arts Inc.
 * Licensed under MIT
 */

/* global module */
/* global require */

module.exports = function (grunt) {
    'use strict';

    // Force use of Unix newlines
    grunt.util.linefeed = '\n';

    RegExp.quote = function (string) {
        return string.replace(/[-\\^$*+?.()|[\]{}]/g, '\\$&');
    };

    // Project configuration.
    grunt.initConfig({

        // Metadata.
        pkg: grunt.file.readJSON('package.json'),

        banner: '/*!\n' +
                        ' * OTK v<%= pkg.version %> (<%= pkg.homepage %>)\n' +
                        ' * Copyright 2011-<%= grunt.template.today("yyyy") %> <%= pkg.author %>\n' +
                        ' * Licensed under <%= pkg.license.type %> (<%= pkg.license.url %>)\n' +
                        ' */\n',

        jqueryCheck: 'if (typeof jQuery === \'undefined\') { throw new Error(\'OTK\\\'s JavaScript requires jQuery\') }\n\n',

        // Task configuration.
        clean: {
            dist: ['dist', 'docs/dist', 'docs/bower_components'],
            server: '.tmp'
        },

        csslint: {
            options: {
                csslintrc: 'less/.csslintrc'
            },
            src: ['dist/otk.css']
        },

        less: {
            compileCore: {
                options: {
                    strictMath: true,
                    sourceMap: true,
                    outputSourceFiles: true,
                    sourceMapURL: '<%= pkg.name %>.css.map',
                    sourceMapFilename: 'dist/<%= pkg.name %>.css.map'
                },
                files: {
                    'dist/<%= pkg.name %>.css': 'less/otk.less'
                }
            },
            minify: {
                options: {
                    cleancss: true,
                    report: 'min'
                },
                files: {
                    'dist/<%= pkg.name %>.min.css': 'dist/<%= pkg.name %>.css'
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
                        'dist/<%= pkg.name %>.css',
                        'dist/<%= pkg.name %>.min.css'
                    ]
                }
            }
        },

        csscomb: {
            options: {
                config: 'less/.csscomb.json'
            },
            dist: {
                files: {
                    'dist/<%= pkg.name %>.css': 'dist/css/<%= pkg.name %>.css'
                }
            }
        },

        copy: {
            fonts: {
                expand: true,
                src: 'fonts/*',
                dest: 'dist/'
            },
            docs: {
                expand: true,
                cwd: './dist',
                src: [
                    '*',
                    'fonts/*'
                ],
                dest: 'docs/dist'
            },
            bower: {
                expand: true,
                src: ['./bower_components/**'],
                dest: 'docs'
            }
        },

        watch: {
            src: {
                files: '<%= jshint.src.src %>',
                tasks: ['dist-docs']
            },
            less: {
                files: 'less/*.less',
                tasks: ['dist-css', 'dist-docs']
            },
            gruntfile: {
                files: ['Gruntfile.js']
            },
            livereload: {
                options: {
                    livereload: '<%= connect.options.livereload %>'
                },
                files: []
            }
        },

        // The actual grunt server settings
        connect: {
            options: {
                port: 9000,
                // Change this to '0.0.0.0' to access the server from outside.
                hostname: '0.0.0.0',
                livereload: 35729,
                base: './docs'
            },
            livereload: {
                options: {
                    open: true,
                    base: './docs'
                }
            },
            test: {
                options: {
                    port: 9001,
                    base: './docs'
                }
            },
            dist: {
                options: {
                    base: './docs'
                }
            }
        },

        // to compress otk into a zip
        compress: {
            main: {
                options: {
                    archive: 'otk.zip'
                },
                expand: true,
                cwd: 'dist/',
                src: ['**/*'],
                dest: '.'
            }
        }

    });


    grunt.registerTask('serve', function (target) {
        if (target === 'dist') {
            return grunt.task.run(['build', 'connect:dist:keepalive']);
        }

        grunt.task.run([
            'clean:server',
            'copy',
            'connect:livereload',
            'watch'
        ]);
    });

    grunt.registerTask('server', function () {
        grunt.log.warn('The `server` task has been deprecated. Use `grunt serve` to start a server.');
        grunt.task.run(['serve']);
    });


    // These plugins provide necessary tasks.
    require('load-grunt-tasks')(grunt, {scope: 'devDependencies'});

    // Test task.
    grunt.registerTask('test', ['dist-css', 'csslint']);

    // CSS distribution task.
    grunt.registerTask('dist-css', ['less', 'csscomb', 'usebanner']);

    // Docs distribution task.
    grunt.registerTask('dist-docs', 'copy:docs');

    // Full distribution task.
    grunt.registerTask('dist', ['clean', 'dist-css', 'copy:fonts', 'copy:bower', 'dist-docs', 'compress']);

    // Default task.
    grunt.registerTask('default', ['test', 'dist']);

};
