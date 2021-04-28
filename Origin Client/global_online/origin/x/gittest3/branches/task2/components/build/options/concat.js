module.exports = function() {
    return {
        dist: {
            src: [
                '.tmp/configs/*.js',
                '<%= yeoman.app %>/*.js',
                '<%= yeoman.app %>/factories/**/*.js',
                '<%= yeoman.app %>/filters/**/*.js',
                '<%= yeoman.app %>/directives/**/scripts/*.js'
            ],
            dest: '<%= yeoman.dist %>/scripts/origincomponents.js'
        }
    };
};