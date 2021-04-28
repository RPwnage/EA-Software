// Performs rewrites based on filerev and the useminPrepare configuration
module.exports = function() {
    return {
        html: ['<%= yeoman.dist %>/{,*/}*.html'],
        css: ['<%= yeoman.dist %>/styles/{,*/}*.css'],
        options: {
            assetsDirs: ['<%= yeoman.dist %>', '<%= yeoman.dist %>/images']
        }
    };
};