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
    templatecompile: {
        options: {
            removeComments: true,
            collapseWhitespace: true
        },
        compile: {
            files: [{
                cwd: 'test/fixtures/directives',
                dest: 'tmp/fixtures/directives',
                src: [
                    'scripts/**/*.js'
                ]
            }]
        }
    },
    nodeunit: {
      tests: ['test/test.js']
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
  grunt.loadNpmTasks('grunt-contrib-nodeunit');
  grunt.loadNpmTasks('grunt-contrib-internal');
  grunt.loadNpmTasks('grunt-jscs');

  grunt.registerTask('test', ['jshint', 'jscs', 'clean', 'templatecompile', 'nodeunit']);
  grunt.registerTask('default', ['test']);
};
