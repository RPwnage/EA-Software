// Generated on 2014-09-03 using generator-angular 0.9.5
'use strict';

// # Globbing
// for performance reasons we're only matching one level down:
// 'test/spec/{,*/}*.js'
// use this if you want to recursively match all subfolders:
// 'test/spec/**/*.js'

module.exports = function(grunt) {
    var modRewrite = require('connect-modrewrite'),
        OPTIONS_LOCATION = './build/options';

    function readOptions(fileName) {
        var options = require(OPTIONS_LOCATION + '/' + fileName);

        return options();
    }

    // Load grunt tasks automatically
    require('load-grunt-tasks')(grunt);

    // Time how long tasks take. Can help when optimizing build times
    require('time-grunt')(grunt);

    // Load the pre minifier dependecy injection tool
    grunt.loadNpmTasks('grunt-ng-annotate');

    // Load ngdocs package
    grunt.loadNpmTasks('grunt-ngdocs');

    // Load configuration builder
    grunt.loadNpmTasks('grunt-ng-constant');

    //merges the origin and non origin configs
    grunt.loadNpmTasks('grunt-merge-json');

    // Configurable paths for the application
    var pathConfig = {
        app: require('./bower.json').appPath || 'src',
        dist: 'dist'
    };

    // Define the configuration for all the tasks
    grunt.initConfig({

        // Project settings
        yeoman: pathConfig,

        // Config module generator
        ngconstant: {
            options: {
                wrap: true
            },
            version: {
                options: {
                    dest: '.tmp/constants/app-version.js',
                    name: 'origin-app-version',
                    constants: {'APP_VERSION': grunt.file.read('build/version.txt')}
                }
            },
            telemetry: {
                options: {
                    dest: '.tmp/constants/telemetry-config.js',
                    name: 'origin-telemetry-config',
                    constants: function() {
                        return {
                            TELEMETRY_CONFIG: grunt.file.readJSON('.tmp/telemetry-config.json')
                        };
                    }
                }
            }

        },

        // The actual grunt server settings
        connect: {
            options: {
                protocol: 'https',
                port: 443,
                key: grunt.file.read('./ssl/localapp.key'),
                cert: grunt.file.read('./ssl/localapp.crt'),
                ca: grunt.file.read('./ssl/ca.crt'),

                // Change this to '0.0.0.0' to access the server from outside.
                hostname: '0.0.0.0',
                livereload: 34729
            },
            livereloadhttps: {
                key: grunt.file.read('./ssl/localapp.key'),
                cert: grunt.file.read('./ssl/localapp.crt'),
                port: 34729
            },
            livereload: {
                options: {
                    open: true,
                    middleware: function(connect) {
                        return [
                            modRewrite(['!\\.html|\\.js|\\.svg|\\.css|\\.png|\\.jpg|\\.jpeg|\\.gif|\\.woff|\\.ttf|\\.eot|\\.json|\\.map|\\.ico$ /index.html [L]']),
                            connect.static('.tmp'),
                            connect().use(
                                '/bower_components',
                                connect.static('./bower_components')
                            ),
                            connect.static(pathConfig.dist)
                        ];
                    }
                }
            },
            test: {
                options: {
                    port: 9002,
                    middleware: function(connect) {
                        return [
                            connect.static('.tmp'),
                            connect.static('test'),
                            connect().use(
                                '/bower_components',
                                connect.static('./bower_components')
                            ),
                            connect.static(pathConfig.dist)

                        ];
                    }
                }
            },
            dist: {
                options: {
                    open: true,
                    base: '<%= yeoman.dist %>'
                }
            }
        },
        'merge-json': readOptions('merge-json.js'),
        watch: readOptions('watch.js'),
        jshint: readOptions('jshint.js'),
        jsdoc: readOptions('jsdoc.js'),
        csscomb: readOptions('csscomb.js'),
        csslint: readOptions('csslint.js'),
        clean: readOptions('clean.js'),
        autoprefixer: readOptions('autoprefixer.js'),
        wiredep: readOptions('wiredep.js'),
        less: readOptions('less.js'),
        filerev: readOptions('filerev.js'),
        useminPrepare: readOptions('usemin-prepare.js'),
        usemin: readOptions('usemin.js'),
        uglify: readOptions('uglify.js'),
        htmlmin: readOptions('htmlmin.js'),
        ngAnnotate: readOptions('ng-annotate.js'),
        ngdocs: readOptions('ngdocs.js'),
        cdnify: readOptions('cdnify.js'),
        copy: readOptions('copy.js'),
        concurrent: readOptions('concurrent.js'),
        concat: readOptions('concat.js'),
        karma: readOptions('karma.js'),
        'json_wrapper': readOptions('jsonwrapper.js'),
        inline: readOptions('inline.js')
    });

    grunt.registerTask('generate-pubint-configs', 'Compile then start a connect web server', function() {
        //build a set configs to be used by 3rd party publishing
        var APPCONFIG_NAME = 'app-config.json',
            COMPONENT_CONFIG_NAME = 'components-config.json',
            JSSDK_CONFIG_NAME = 'jssdk-origin-config.json',
            BREAK_POINTS_CONFIG = 'breakpoints-config.json',
            CURRENT_CONFIG_PATH = '../tools/config/',
            DEST_CONFIG_PATH = './dist/configs/pubint/',
            ENVIRONMENT = 'environment/integration',
            configs = {},
            jssdkConfig = {},
            files = [
                APPCONFIG_NAME,
                COMPONENT_CONFIG_NAME,
                JSSDK_CONFIG_NAME,
                BREAK_POINTS_CONFIG
            ],
            //we have a very specific set of overrides that we want to move to a data1 endpoint
            jssdkOverrideIds =
            [
                'catalogInfo',
                'catalogInfoLMD',
                'criticalCatalogInfo',
                'offerIdbyPath',
                'basegameOfferIdByMasterTitleId',
                'ratingsOffers',
                'ratingsBundle',
                'anonRatingsOffers',
                'anonRatingsBundle'
            ];

        //read in each config file and modify the basedata to point to prod
        files.forEach(function(item) {
            configs[item] = JSON.parse(grunt.file.read(CURRENT_CONFIG_PATH + item));
            if(configs[item].hostname && configs[item].hostname.basedata) {
                configs[item].hostname.basedata = 'https://data1.origin.com/';
            }
        });

        //locate the jssdk config file
        jssdkConfig = configs[JSSDK_CONFIG_NAME];

        //modify the jssdk base API urls
        jssdkOverrideIds.forEach(function(item) {
            jssdkConfig.urls[item] = jssdkConfig.urls[item].replace('{baseapi}', 'https://api{num}.origin.com/');
        });

        //write out each file
        files.forEach(function(item) {
            grunt.file.write(DEST_CONFIG_PATH + item, JSON.stringify(configs[item]));
        });

        //copy the environment folder
          grunt.file.recurse(CURRENT_CONFIG_PATH + ENVIRONMENT , function(abspath, rootdir, subdir, filename) {
            grunt.file.copy(abspath, DEST_CONFIG_PATH + ENVIRONMENT +'/' + filename);
          });          
    });

    grunt.registerTask('serve', 'Compile then start a connect web server', function(target) {
        if (target === 'dist') {
            return grunt.task.run(['build', 'connect:dist:keepalive']);
        }

        if (target === 'nobuild') {
            return grunt.task.run([
                'clean:server',
                'connect:livereload',
                'watch'
            ]);
        }

        grunt.task.run([
            'dev',
            'clean:server',
            'connect:livereload',
            'watch'
        ]);
    });

    grunt.registerTask('server', 'DEPRECATED TASK. Use the "serve" task instead', function(target) {
        grunt.log.warn('The `server` task has been deprecated. Use `grunt serve` to start a server.');
        grunt.task.run(['serve:' + target]);
    });


    // need this for the vm as we do not want livereload
    grunt.registerTask('watch:vm', function() {
        var config = {
            vm: {
                files: [
                    '<%= yeoman.app %>/**',
                    'test/fixtures/components.dict.json'
                ],
                tasks: ['setup-vm']
            }
        };
        grunt.config('watch', config);
        grunt.task.run('watch');
    });

    grunt.registerTask('test', [
        'clean:server',
        'merge-json',
        'ngconstant:telemetry',    
        'autoprefixer',
        'karma:unit'
    ]);

    // CSS distribution task.
    grunt.registerTask('dist-css', ['less', 'csscomb']);

    // tasks specific for the setup script as per Richard's request
    grunt.registerTask('setup-vm', [
        'clean:dist',
        'less',
        'merge-json',
        'ngconstant:version',
        'ngconstant:telemetry',
        'copy:dev',
        'copy:preload',
        'copy:ngconstants',
        'copy:scripts',
        'copy:styles',
        'copy:images',
        'copy:icons',
        'copy:configs',
        'json_wrapper',
        'inline',
        'generate-pubint-configs'
    ]);

    grunt.registerTask('build', [
        'clean:dist',
        'merge-json',
        'ngconstant:version',
        'ngconstant:telemetry',
        'copy:configs',
        'dist-css',
        'useminPrepare',
        'concurrent:dist',
        'autoprefixer',
        'concat',
        'ngAnnotate',
        'copy:dist',
        'copy:preload',
        'copy:icons',
        'copy:componentimages',
        'json_wrapper',
        'inline',
        'cssmin',
        'uglify',
        'filerev',
        'usemin',
        'htmlmin',
        'generate-pubint-configs'        
    ]);

    grunt.registerTask('default', [
        'newer:jshint',
        'dist-css',
        'csslint',
        'test',
        'build'
    ]);

    grunt.registerTask('dev', [
        'clean:dist',
        'merge-json',
        'ngconstant:version',
        'ngconstant:telemetry',
        'newer:jshint',
        'dist-css',
        'csslint',
        'copy:dev',
        'copy:preload',
        'copy:ngconstants',
        'copy:configs',
        'copy:scripts',
        'copy:styles',
        'copy:images',
        'copy:icons',
        'json_wrapper',
        'inline',
        'generate-pubint-configs',

    ]);

    grunt.registerTask('docs', [
        'ngdocs',
        'copy:docs'
    ]);


};
