// Watches files for changes and runs tasks based on the changed files
module.exports = function() {
    return {
        js: {
            files: [
                '<%= yeoman.app %>/component-config.js',
                '<%= yeoman.app %>/{,*/}*.js',
                '<%= yeoman.app %>/directives/**/scripts/{,*/}*.js',
                '<%= yeoman.app %>/directives/**/test/scripts/{,*/}*.js',
                '<%= yeoman.app %>/factories/**/{,*/}*.js',
                '<%= yeoman.app %>/filters/**/{,*/}*.js'
            ],
            tasks: ['build'],
            options: {
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