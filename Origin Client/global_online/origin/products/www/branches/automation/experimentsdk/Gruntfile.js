/*global module:false*/
module.exports = function(grunt) {
    var OPTIONS_LOCATION = './build/options';
    var configReader = require('./tools/buildUtils/config-reader.js')(grunt);

    function readOptions(fileName) {
        var options = require(OPTIONS_LOCATION + '/' + fileName);

        return options();
    }

    grunt.loadNpmTasks('grunt-contrib-jshint');
    grunt.loadNpmTasks('grunt-contrib-copy');
    grunt.loadNpmTasks('grunt-contrib-watch');
    grunt.loadNpmTasks('grunt-jsdoc');
    grunt.loadNpmTasks('grunt-contrib-jasmine');
    grunt.loadNpmTasks('grunt-contrib-requirejs');
    grunt.loadNpmTasks('grunt-merge-json');
    grunt.loadNpmTasks('grunt-karma');


    grunt.initConfig({
        pkg: grunt.file.readJSON('package.json'),
        copy: readOptions('copy.js'),
        watch: {
            js: {
                files: ['src/**/*.js'],
                tasks: ['requirejs:dev']
            }
        },
        requirejs: readOptions('compile-requirejs.js'),
        jshint: readOptions('jshint.js'),
        jsdoc: readOptions('jsdoc.js'),
        'merge-json': readOptions('merge-json.js'),
        karma: {
            unit: {
                configFile: 'karma.conf.js',
                singleRun: true

            }
        }
    });

    grunt.registerTask('dev', [
        'jshint',
        'buildExpConfigFile',
        'requirejs:dev',
        'karma'
    ]);

    grunt.registerTask('release', [
        'jshint',
        'buildExpConfigFile',
        'requirejs:minified',
        'karma'
    ]);

    grunt.registerTask('nologs', [
        'jshint',
        'buildExpConfigFile',
        'requirejs:nologs'
    ]);

    grunt.registerTask('docs', [
        'checkEXPSDKExists',
        'copy:main',
        'jsdoc:dist',
        'copy:post'
    ]);
/*
    grunt.registerTask('docs-copyapp', [
        'docs',
        'copy:docstoappdist'
    ]);

    grunt.registerTask('internal-docs', [
        'checkJSSDKExists',
        'copy:main',
        'jsdoc:internal',
        'copy:post'
    ]);

*/
    grunt.registerTask('test', [
        'karma'
    ]);

    grunt.registerTask('buildExpConfigFile', [
        'merge-json',
        'createConfigModule'
    ]);

    grunt.registerTask('default', [
        'dev'
    ]);

    grunt.registerTask('setup-vm', [
        'dev'
    ]);

    grunt.registerTask('checkEXPSDKExists', 'checks for eax-experimentsdk.js', function() {
        if(!grunt.file.exists('dist/eax-experimentsdk.js')) {
            grunt.fail.warn('You must first build the EXPSDK to build docs');
        }
    });

    grunt.registerTask('createConfigModule', 'Generates the Exp constant file', function() {
        var config = configReader.readConfig({id:'experimentsdkconfig', filename:'tmp/experimentsdk-config.json'}),
        filepath = 'src/generated/experimentsdkconfig.js';
        grunt.file.write(filepath, '/*jshint strict: false */define([], function () { return ' + JSON.stringify(config.constants.experimentsdkconfig).replace(/"/g, "'") + ';});', {force:true});
    });

    grunt.registerTask('watch:vm', 'Watcher daemon specific to the virtual machine (headless).', function(target) {
        grunt.task.run(['watch:js']);
    });
};