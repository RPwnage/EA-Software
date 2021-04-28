// Copies remaining files to places other tasks can use

module.exports = function() {

    'use strict';

    return {
        dist: {
            files: [{
                expand: true,
                dot: true,
                filter: 'isFile',
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: ['directives/**/views/*.html']
            }, {
                expand: true,
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: ['images/{,*/}*.{png,jpg,jpeg,gif}']
            }, {
                expand: true,
                cwd: '.tmp/styles',
                dest: '<%= yeoman.dist %>/styles',
                src: ['*']
            }]
        },
        'karma-test': {
            files: [{
                expand: true,
                cwd: '<%= yeoman.app %>/directives',
                dest: '.tmp/tc_directives',
                src: [
                    '**/*.js'
                ]
            }]
        }
    };
};