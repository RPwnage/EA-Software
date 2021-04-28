// Empties folders to start fresh

module.exports = function() {
    return {
        dist: {
            files: [{
                dot: true,
                src: [
                    '.tmp',
                    '<%= yeoman.dist %>/*',
                    '!<%= yeoman.dist %>/.git*',
                    '!<%= yeoman.dist %>/bower_components/**'
                ]
            }]
        },
        server: '.tmp'
    };
};