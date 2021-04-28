//Link together the factories and directives into a single library includable

module.exports = function() {

    'use strict';

    return {
        dist: {
            src: [
                '.tmp/configs/*.js',
                '<%= yeoman.app %>/*.js',
                '<%= yeoman.app %>/helpers/**/*.js',
                '<%= yeoman.app %>/patterns/**/*.js',
                '<%= yeoman.app %>/models/**/*.js',
                '<%= yeoman.app %>/refiners/**/*.js',
                '<%= yeoman.app %>/factories/**/*.js',
                '<%= yeoman.app %>/filters/**/*.js',
                '<%= yeoman.app %>/directives/**/*.js',
                '.tmp/templates.js'
            ],
            dest: '<%= yeoman.dist %>/scripts/origincomponents.js'
        },
        "karma-test": {
            src: [
                '.tmp/configs/*.js',
                'test/framework/origin-components-standalone.js',
                '<%= yeoman.app %>/helpers/**/*.js',
                '<%= yeoman.app %>/patterns/**/*.js',
                '<%= yeoman.app %>/models/**/*.js',
                '<%= yeoman.app %>/refiners/**/*.js',
                '<%= yeoman.app %>/factories/**/*.js',
                '<%= yeoman.app %>/filters/**/*.js',
                '<%= yeoman.app %>/directives/**/*.js'
            ],
            dest: '.tmp/karma-test/origin-test-components.js'
        }
    };
};