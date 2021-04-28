'use strict';

module.exports = function(grunt) {
  grunt.initConfig({
    cqconfig: {
        "packageName": "eaxexp-cq5-angular-components",
        "encoding": "utf-8",
        "packageDestinationPath": "tmp/cq5",

        "sidekickGroupName": "Web: ",
        "componentsAppsCrxPath": "/apps/originx/eax/components/imported",
        "componentsDialogsCrxPath": "/apps/originx/eax/components/imported/dialogs",
        "componentsDirectivesCrxPath": "/apps/originx/eax/components/imported/directives",

        "defaultsAppsCrxPath": "/apps/eax/originx-defaults",
        "defaultsDesignCrxPath": "/etc/eax/designs/originx-defaults",
        "defaultsContentCrxPath": "/content/eax/web-defaults/defaults",

        "devAppsCrxPath": "/apps/eax/originx-dev",
        "devDesignCrxPath": "/etc/eax/designs/originx-dev",
        "devContentCrxPath": "/content/eax/web-dev/dev"
    },
    clean: {
      test: ['tmp']
    },
    copy: {
        'cq5-base': {
            expand: true,
            dot: true,
            cwd: 'build/cq5/package',
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
                cwd: 'build/cq5/package/META-INF/vault',
                src: ['**'],
                dest: '<%= cqconfig.packageDestinationPath %>/META-INF/vault/'
            }]
        }
    },
    'package-cq5': {
        compile: {
            options: {
                "fixtureFile": 'src/fixture/eaxexp.dict.json',
                "skipDefault": true,
                "overwriteExistingDefaultValue" : false,
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
                cwd: 'src/directives',
                dest: '<%= cqconfig.packageDestinationPath %>/jcr_root',
                src: [
                    '**/*.js'
                ]
            }]
        }
    },
    'check-enumerations': {
        compile: {
          src: ['src/directives/**/*.js']
        }
    },
    "package-p4ignore": {
        compile: {
            files: [{
                dot: true,
                cwd: 'tmp/cq5/jcr_root',
                dest: 'tmp/cq5/jcr_root',
                src: '**'
            }]
        }
    },
    "package-gitignore": {
        compile: {
            files: [{
                dot: true,
                cwd: 'tmp/cq5/jcr_root',
                dest: 'tmp/cq5/jcr_root',
                src: '**'
            }]
        }
    },
    compress: {
        zip: {
            options: {
                archive: function() {
                    return 'tmp/eaxexp-cq5-angular-components.zip';
                }
            },
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= cqconfig.packageDestinationPath %>',
                src: ['**/*']
            }]
        }
    }
  });

  grunt.loadNpmTasks('grunt-contrib-clean');
  grunt.loadNpmTasks('grunt-contrib-internal');
  grunt.loadNpmTasks('grunt-contrib-copy');
  grunt.loadNpmTasks('grunt-contrib-compress');
  grunt.loadNpmTasks('grunt-replace');
//  grunt.loadNpmTasks('origin-grunt-component-packager');
  grunt.loadNpmTasks('eaxexp-grunt-component-packager');
  grunt.loadNpmTasks('origin-grunt-component-packager-dev-tools');
  grunt.registerTask('build', ['clean', 'copy:cq5-base', 'replace:paths', 'check-enumerations','package-cq5', 'package-p4ignore', 'package-gitignore', 'compress:zip']);
  grunt.registerTask('default', ['build']);
};
