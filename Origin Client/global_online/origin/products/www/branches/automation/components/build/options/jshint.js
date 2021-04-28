// Make sure code styles are up to par and there are no obvious mistakes

module.exports = function() {
    return {
        options: {
            jshintrc: '.jshintrc',
            reporter: require('jshint-stylish')
        },
        single: [], //config used for the js watch
        all: [
            'Gruntfile.js',
            '<%= yeoman.app %>/directives/**/scripts/{,*/}*.js',
            '<%= yeoman.app %>/directives/*/*test/scripts/{,*/}*.js',
            '<%= yeoman.app %>/helpers/**/*.js',
            '<%= yeoman.app %>/patterns/**/*.js',
            '<%= yeoman.app %>/models/**/*.js',
            '<%= yeoman.app %>/refiners/**/*.js',
            '<%= yeoman.app %>/factories/**/*.js',
            '<%= yeoman.app %>/filters/**/{,*/}*.js'
        ],
        test: {
            options: {
                jshintrc: 'test/.jshintrc'
            },
            src: ['test/spec/{,*/}*.js']
        }
    };
};