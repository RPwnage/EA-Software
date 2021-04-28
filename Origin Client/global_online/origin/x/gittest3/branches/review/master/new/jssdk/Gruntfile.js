/*global module:false*/
module.exports = function(grunt) {
    var OPTIONS_LOCATION = './build/options';
    var configReader = require('../tools/buildUtils/config-reader.js')(grunt);

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
        jasmine: readOptions('test-jasmine.js')
    });

    grunt.registerTask('dev', [
        'jshint',
        'jsConfig',
        'requirejs:dev'
    ]);

    grunt.registerTask('release', [
        'jshint',
        'jsConfig',
        'requirejs:minified'
    ]);

    grunt.registerTask('docs', [
        'checkJSSDKExists',
        'copy:main',
        'jsdoc:dist',
        'copy:post'
    ]);

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

    grunt.registerTask('functional-test', [
        'start-mockup',
        'jasmine:test',
        'terminate-mockup'
    ]);

    grunt.registerTask('default', [
        'dev'
    ]);

    grunt.registerTask('setup-vm', [
        'dev'
    ]);

    var mockserverProc = null;

    grunt.registerTask('checkJSSDKExists', 'checks for origin-jssdk.js', function() {
        if(!grunt.file.exists('dist/origin-jssdk.js')) {
            grunt.fail.warn('You must first build the JSSDK to build docs');
        }
    });

    grunt.registerTask('jsConfig', 'Generates the JS constant file', function() {
        var config = configReader.readConfig({id:'jsconfig', filename:'config/config.json'}),
        filepath = 'src/generated/jssdkconfig.js';

        grunt.file.write(filepath, '/*jshint strict: false */define([], function () { return ' + JSON.stringify(config.constants.jsconfig).replace(/"/g, "'") + ';});', {force:true});
    });

    grunt.registerTask('start-mockup', 'Starts mockup server on background', function() {
        var sh = require('shelljs');
        sh.cd('../automation/mockup/');
        sh.exec('npm install');
        sh.cd('../../jssdk/');

        var spawn = require('child_process').spawn;
        mockserverProc = spawn('node', ['src/start.js'], {
            cwd: '../automation/mockup/'
        });
        mockserverProc.stderr.on('data', function(data) {
            console.log('mockup err: ' + data);
        });
    });

    grunt.registerTask('terminate-mockup', 'Terminates mockup server', function() {
        grunt.task.requires('start-mockup');
        if (mockserverProc) {
            process.kill(mockserverProc.pid);
        }
    });

    grunt.registerTask('watch:vm', 'Watcher daemon specific to the virtual machine (headless).', function(target) {
        grunt.task.run(['watch:js']);
    });
};