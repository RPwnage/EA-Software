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
  grunt.loadNpmTasks('origin-grunt-component-packager');
  grunt.loadNpmTasks('origin-grunt-component-packager-dev-tools');
  grunt.registerTask('build', ['clean', 'copy:cq5-base', 'replace:paths', 'package-cq5', 'package-p4ignore', 'compress:zip']);
  grunt.registerTask('default', ['build']);
};
