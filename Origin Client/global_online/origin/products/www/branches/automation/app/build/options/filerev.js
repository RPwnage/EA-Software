// Renames files for browser caching purposes

module.exports = function() {
    return {
        dist: {
            src: [
                '<%= yeoman.dist %>/scripts/{,*/}*.js',
                '<%= yeoman.dist %>/styles/{,*/}*.css',
            ]
        }
    };
};