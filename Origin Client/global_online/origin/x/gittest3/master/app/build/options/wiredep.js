// Automatically inject Bower components into the app

module.exports = function() {
    return {
        app: {
            src: ['<%= yeoman.app %>/index.html'],
            ignorePath: /\.\.\//
        }
    };
};