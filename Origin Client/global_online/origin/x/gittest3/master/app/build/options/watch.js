// Watches files for changes and runs tasks based on the changed files

module.exports = function() {

    var appFiles = [
        'src/routing/**/*.js',
        'src/controllers/**/*.js',
        'src/factories/**/*.js',
        'src/bootstrap/**/*.js',
        'src/*.js'
    ];

    return {
        bowerOrigin: {
            files: [
                'dist/bower_components/origin-components/dist/scripts/*.js',
                'dist/bower_components/origin-ui-toolkit/dist/*.js',
                'dist/bower_components/origin-jssdk/dist/*.js',
                'dist/bower_components/origin-i18n/dist/*.js'
            ],
            options: {
                livereload: '<%= connect.livereloadhttps %>'
            }
        },
        js: {
            files: appFiles,
            tasks: ['newer:jshint:all', 'copy:scripts'],
            options: {
                livereload: '<%= connect.livereloadhttps %>'
            }
        },
        jsTest: {
            files: ['test/spec/{,*/}*.js'],
            tasks: ['newer:jshint:test', 'karma']
        },
        styles: {
            files: ['<%= yeoman.app %>/less/{,*/}*.less'],
            tasks: ['less', 'autoprefixer', 'copy:styles'],
            options: {
                livereload: '<%= connect.livereloadhttps %>'
            }
        },
        gruntfile: {
            files: ['Gruntfile.js']
        },
        livereload: {
            options: {
                livereload: '<%= connect.livereloadhttps %>'
            },
            files: [
                '<%= yeoman.app %>/{,*/}*.html',
                '.tmp/styles/{,*/}*.css'
            ],
            tasks: ['copy:dev:files'],
        }
    };
};