'use strict';

module.exports = function(grunt) {
    require('time-grunt')(grunt);

    grunt.initConfig({
        concat: {
            dist: {
                files: [{
                    dest: 'dist/origin-locale.js',
                    src: [
                        'src/wrapper/intro.js',
                        'src/processor/**.js',
                        'src/api/api.js',
                        'src/parser/parser.js',
                        'src/wrapper/outro.js'
                    ]
                }]
            }
        },
        karma: {
            unit: {
                configFile: 'karma.conf.js',
                singleRun: true
            }
        },
        jscs: {
            src: [
                'dist/origin-locale.js',
                'Gruntfile.js'
            ],
            options: {
                config: ".jscs.json"
            }
        },
        jshint: {
            options: {
                jshintrc: '.jshintrc'
            },
            all: [
                'Gruntfile.js',
                'dist/origin-locale.js'
            ]
        },
        clean: {
            dirs: ['tmp', 'dist']
        }
    });

    grunt.loadNpmTasks('grunt-contrib-clean');
    grunt.loadNpmTasks('grunt-contrib-jshint');
    grunt.loadNpmTasks('grunt-jscs');
    grunt.loadNpmTasks('grunt-contrib-concat');
    grunt.loadNpmTasks('grunt-karma');

    grunt.registerTask('default', [
        'clean:dirs',
        'concat:dist',
        'jshint',
        'jscs',
        'karma'
    ]);
};
