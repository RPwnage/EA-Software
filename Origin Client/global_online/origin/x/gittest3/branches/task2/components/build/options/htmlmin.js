module.exports = function() {
    return {
        dist: {
            options: {
                collapseWhitespace: true,
                collapseBooleanAttributes: true,
                removeCommentsFromCDATA: true,
                removeOptionalTags: true
            },
            files: [{
                expand: true,
                cwd: '<%= yeoman.dist %>/views/',
                src: ['*.html'],
                dest: '<%= yeoman.dist %>/views/'
            }]
        }
    };
};