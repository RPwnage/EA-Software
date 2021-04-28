'use strict';

module.exports = function(grunt) {
  grunt.initConfig({
    devtoolsconfig: {
        "p4ignoreFileName": ".p4ignore"
    },
    "package-p4ignore": {
        compile: {
            files: [{
                dot: true,
                cwd: 'test/fixtures/cq5-build/jcr_root',
                dest: 'tmp/cq5',
                src: '**'
            }]
        }
    },
    jshint: {
      options: {
        jshintrc: '.jshintrc'
      },
      all: [
        'Gruntfile.js',
        'tasks/*.js',
        'tasks/lib/*.js',
        '<%= nodeunit.tests %>'
      ]
    },
    clean: {
      test: ['tmp']
    },
    copy: {
        'cq5-base': {
            expand: true,
            dot: true,
            cwd: 'tasks/package',
            src: '**',
            dest: '<%= cqconfig.packageDestinationPath %>'
        }
    },
    nodeunit: {
      tests: ['test/*.js']
    },
    jscs: {
      src: ['tasks/*.js',
        'tasks/lib/*.js',
        'test/fixtures/directives/scripts/*.js',
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
  grunt.loadNpmTasks('grunt-contrib-copy');

  grunt.registerTask('test', ['clean', 'jshint', 'jscs', 'package-p4ignore', 'nodeunit']);
  grunt.registerTask('default', ['test']);
};
