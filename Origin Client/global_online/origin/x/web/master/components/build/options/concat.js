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
        },
        "karma-test": {
            src: [
                '.tmp/configs/*.js',
                'test/framework/origin-components-standalone.js',
                '<%= yeoman.app %>/factories/**/*.js',
                '<%= yeoman.app %>/filters/**/*.js',
                '.tmp/tc_directives/**/scripts/*.js'
            ],
            dest: '.tmp/karma-test/origin-test-components.js'
        }
    };
};