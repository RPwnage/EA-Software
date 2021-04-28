'use strict';

module.exports = function(grunt) {
  grunt.initConfig({
    cqconfig: {
        "packageName": "origin-cq5-angular-components",
        "encoding": "utf-8",
        "packageDestinationPath": "tmp/cq5",

        "sidekickGroupName": "Web: ",
        "componentsAppsCrxPath": "/apps/originx/components/imported",
        "componentsDialogsCrxPath": "/apps/originx/components/imported/dialogs",
        "componentsDirectivesCrxPath": "/apps/originx/components/imported/directives",

        "defaultsAppsCrxPath": "/apps/originx-defaults",
        "defaultsDesignCrxPath": "/etc/designs/originx-defaults",
        "defaultsContentCrxPath": "/content/web-defaults/defaults",

        "devAppsCrxPath": "/apps/originx-dev",
        "devDesignCrxPath": "/etc/designs/originx-dev",
        "devContentCrxPath": "/content/web-dev/dev"
    },
    jshint: {
      options: {
        jshintrc: '.jshintrc'
      },
      all: [
        'Gruntfile.js',
        'tasks/*.js',
        'tasks/lib/*.js',
        'test/fixtures/directives/scripts/*.js',
        '<%= nodeunit.tests %>'
      ]
    },
    clean: {
      test: ['tmp']
    },
    ngdocs: {
        options: {
            dest: 'tmp/docs',
            html5Mode: false,
            title: 'Fixture Documentation',
            bestMatch: true,
            editExample: false
        },
        all: {
            src: [
                'test/fixtures/directives/scripts/example-directive.js'
            ]
        }
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
    replace: {
        paths: {
            options: {
                variables: {
                    "packageName": "<%= cqconfig.packageName %>",
                    "encoding": "<%= cqconfig.encoding %>",
                    "packageDestinationPath": "<%= cqconfig.packageDestinationPath %>",

                    "sidekickGroupName": "<%= cqconfig.sidekickGroupName %>",
                    "componentsAppsCrxPath": "<%= cqconfig.componentsAppsCrxPath %>",
                    "componentsDialogsCrxPath": "<%= cqconfig.componentsDialogsCrxPath %>",
                    "componentsDirectivesCrxPath": "<%= cqconfig.componentsDirectivesCrxPath %>",

                    "defaultsAppsCrxPath": "<%= cqconfig.defaultsAppsCrxPath %>",
                    "defaultsDesignCrxPath": "<%= cqconfig.defaultsDesignCrxPath %>",
                    "defaultsContentCrxPath": "<%= cqconfig.defaultsContentCrxPath %>",

                    "devAppsCrxPath": "<%= cqconfig.devAppsCrxPath %>",
                    "devDesignCrxPath": "<%= cqconfig.devDesignCrxPath %>",
                    "devContentCrxPath": "<%= cqconfig.devContentCrxPath %>"
                }
            },
            files: [{
                expand: true,
                dot: true,
                cwd: 'tasks/package/META-INF/vault',
                src: ['**'],
                dest: '<%= cqconfig.packageDestinationPath %>/META-INF/vault/'
            }]
        }
    },
    'package-cq5': {
        compile: {
            options: {
                "fixtureFile": 'src/fixture/components.dict.json',
                "skipDefault": false,
                "overwriteExistingDefaultValue": true,
                "packageName": "<%= cqconfig.packageName %>",
                "encoding": "<%= cqconfig.encoding %>",
                "packageDestinationPath": "<%= cqconfig.packageDestinationPath %>",

                "sidekickGroupName": "<%= cqconfig.sidekickGroupName %>",
                "componentsAppsCrxPath": "<%= cqconfig.componentsAppsCrxPath %>",
                "componentsDialogsCrxPath": "<%= cqconfig.componentsDialogsCrxPath %>",
                "componentsDirectivesCrxPath": "<%= cqconfig.componentsDirectivesCrxPath %>",

                "defaultsAppsCrxPath": "<%= cqconfig.defaultsAppsCrxPath %>",
                "defaultsDesignCrxPath": "<%= cqconfig.defaultsDesignCrxPath %>",
                "defaultsContentCrxPath": "<%= cqconfig.defaultsContentCrxPath %>",

                "devAppsCrxPath": "<%= cqconfig.devAppsCrxPath %>",
                "devDesignCrxPath": "<%= cqconfig.devDesignCrxPath %>",
                "devContentCrxPath": "<%= cqconfig.devContentCrxPath %>"
            },
            files: [{
                cwd: 'test/fixtures/directives',
                dest: '<%= cqconfig.packageDestinationPath %>/jcr_root',
                src: [
                    'scripts/example-directive.js'
                ]
            }]
        }
    },
    'check-enumerations': {
        compile: {
          src: ['tasks/*.js',
            'tasks/lib/*.js',
            'test/fixtures/directives/scripts/*.js']
        }
    },
    compress: {
        zip: {
            options: {
                archive: function() {
                    return 'tmp/origin-cq5-angular-components.zip';
                }
            },
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= cqconfig.packageDestinationPath %>',
                src: ['**/*']}
            ]}
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
  grunt.loadNpmTasks('grunt-contrib-compress');
  grunt.loadNpmTasks('grunt-replace');
  grunt.loadNpmTasks('grunt-ngdocs');

  grunt.registerTask('test', ['clean', 'ngdocs', 'nodeunit', 'jshint', 'jscs', 'check-enumerations','copy:cq5-base', 'replace:paths', 'package-cq5', 'compress:zip']);
  grunt.registerTask('default', ['test']);
};
