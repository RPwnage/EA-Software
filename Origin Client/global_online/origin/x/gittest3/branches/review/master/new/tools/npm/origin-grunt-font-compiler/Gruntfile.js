'use strict';
module.exports = function(grunt) {
  grunt.initConfig({
    jshint: {
      options: {
        jshintrc: '.jshintrc'
      },
      all: [
        'Gruntfile.js',
        'tasks/*.js',
        '<%= nodeunit.tests %>'
      ]
    },
    clean: {
      test: ['tmp']
    },
    fontcompile: {
        compile: {
            files: [{
                cwd: 'test',
                dest: 'tmp',
                src: [
                    'fixtures/less/fonts.less'
                ]
            }]
        }
    },
    nodeunit: {
      tests: ['test/test.js']
    },
    less: {
        compileCore: {
            options: {
                strictMath: true
            },
            files: {
                'tmp/fonts.css': 'tmp/fixtures/less/fonts.less'
            }
        },
        minify: {
            options: {
                cleancss: true,
                report: 'min'
            },
            files: {
                'tmp/fonts.min.css': 'tmp/fonts.css'
            }
        }
    },
    jscs: {
      src: ['tasks/*.js',
        'Gruntfile.js'],
      options: {
        config: ".jscs.json"
      }
    }
  });

  grunt.loadTasks('tasks');
  grunt.loadNpmTasks('grunt-contrib-clean');
  grunt.loadNpmTasks('grunt-contrib-jshint');
  grunt.loadNpmTasks('grunt-jscs');
  grunt.loadNpmTasks('grunt-contrib-nodeunit');
  grunt.loadNpmTasks('grunt-contrib-internal');
  grunt.loadNpmTasks('grunt-contrib-less');

  grunt.registerTask('test', ['jshint', 'jscs', 'clean', 'fontcompile', 'less', 'nodeunit']);
  grunt.registerTask('default', ['test']);
};
