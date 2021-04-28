// Empties folders to start fresh

module.exports = function() {
    return {
        dist: {
            files: [{
                dot: true,
                src: [
                    '.tmp',
                    '<%= yeoman.dist %>/*.*',
                    '<%= yeoman.dist %>/bootstrap/**',
                    '<%= yeoman.dist %>/configs/**',
                    '<%= yeoman.dist %>/constants/**',
                    '<%= yeoman.dist %>/controllers/**',
                    '<%= yeoman.dist %>/directives/**',
                    '<%= yeoman.dist %>/factories/**',
                    '<%= yeoman.dist %>/images/**',
                    '<%= yeoman.dist %>/routing/**',
                    '<%= yeoman.dist %>/styles/**',
                    '<%= yeoman.dist %>/views/**',
                    '<%= yeoman.dist %>/telemetry/**'
                ]
            }]
        },
        docs: {
            files: [{
                src: [
                    '<%= yeoman.dist %>/docs',
                    'docs'
                ]
            }]
        },
        server: '.tmp'
    };
};