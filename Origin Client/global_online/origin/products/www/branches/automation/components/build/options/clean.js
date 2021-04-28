// Empties folders to start fresh

module.exports = function() {
    return {
        dist: {
            files: [{
                dot: true,
                src: [
                    '.tmp',
                    '<%= yeoman.dist %>/directives/**',
                    '<%= yeoman.dist %>/images/**',
                    '<%= yeoman.dist %>/scripts/**',
                    '<%= yeoman.dist %>/styles/**',
                ]
            }]
        },
        server: '.tmp'
    };
};