module.exports = function() {
    return {
        dist: {
            options: {
                collapseWhitespace: true,
                conservativeCollapse: true,
                collapseBooleanAttributes: true,
                removeCommentsFromCDATA: true,
                removeOptionalTags: true,
                removeComments: true,
                minifyJS: true
            },
            files: [{
                expand: true,
                cwd: '<%= yeoman.dist %>',
                src: ['*.html', '**/views/{,*/}*.html',
                    '!bower_components/origin-components/src/**/*.html',
                    '!bower_components/origin-ui-toolkit/src/**/*.html',
                    '!bower_components/origin-jssdk/dist/src/**/*.html',
                    '!bower_components/origin-i18n/dist/src/**/*.html'
                ],
                dest: '<%= yeoman.dist %>'
            }]
        }
    };
};