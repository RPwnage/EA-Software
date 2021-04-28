// Watches files for changes and runs tasks based on the changed files
module.exports = function(grunt) {


    //watching for the file that changed and running that file only through jshint
    //for this to work spawn must be set to false in the options
    grunt.event.on('watch', function(action, filepath) {
        grunt.config(['jshint', 'single'], [filepath]);
    });

    return {
        html: {
            files: [
                '<%= yeoman.app %>/directives/**/views/{,*/}*.html',
                '<%= yeoman.app %>/directives/**/test/views/{,*/}*.html'
            ],
            tasks: [
                'ngtemplates',
                'copy:dist-assets'
            ]
        },
        js: {
            files: [
                '<%= yeoman.app %>/component-config.js',
                '<%= yeoman.app %>/{,*/}*.js',
                '<%= yeoman.app %>/directives/**/scripts/{,*/}*.js',
                '<%= yeoman.app %>/directives/**/test/scripts/{,*/}*.js',
                '<%= yeoman.app %>/helpers/**/*.js',
                '<%= yeoman.app %>/patterns/**/*.js',
                '<%= yeoman.app %>/models/**/*.js',
                '<%= yeoman.app %>/refiners/**/*.js',
                '<%= yeoman.app %>/factories/**/*.js',
                '<%= yeoman.app %>/filters/**/{,*/}*.js'
            ],
            tasks: [
                'jshint:single',
                'concat:dist'
            ],
            options: {
                spawn: false, //do not spawn task run in child process, otherwise we cannot set the grunt.config for the jshint.single dynamically
                livereload: 35728
            }
        },
        jsTest: {
            files: ['test/spec/{,*/}*.js'],
            tasks: ['newer:jshint:test', 'karma']
        },
        styles: {
            files: ['<%= yeoman.app %>/less/{,*/}*.less'],
            tasks: ['build']
        },
        gruntfile: {
            files: ['Gruntfile.js']
        },
        livereload: {
            options: {
                livereload: 35728
            },
            files: [
                '<%= yeoman.app %>/{,*/}*.html',
                '.tmp/styles/{,*/}*.css',
                '<%= yeoman.app %>/images/{,*/}*.{png,jpg,jpeg,gif,webp,svg}'
            ]
        }
    };
};