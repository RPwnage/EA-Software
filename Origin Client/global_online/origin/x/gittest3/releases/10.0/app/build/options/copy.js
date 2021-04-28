// Copies remaining files to places other tasks can use

module.exports = function() {
    return {
        dist: {
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: [
                    '*.{ico,png,txt}',
                    '.htaccess',
                    '*.html',
                    '*.info',
                    'views/{,*/}*.html',
                    'images/{,*/}*.{webp}'
                ]
            }]
        },
        dev: {
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: [
                    '*.{ico,png,txt}',
                    '.htaccess',
                    '*.html',
                    'views/{,*/}*.html',
                    'images/*',
                ]
            }]
        },
        bower: {
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: [
                    'bower_components/**',
                    '!bower_components/origin-*/**'
                ]
            }]
        },
        bowerOrigin: {
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: [
                    'bower_components/origin-*/dist/**'
                ]
            }]
        },
        scripts: {
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: [
                    'routing/**/*.js',
                    'controllers/**/*.js',
                    'factories/**/*.js',
                    'bootstrap/**/*.js',
                    '*.js'
                ]
            }]
        },
        configs: {
            files: [{
                expand: true,
                cwd: '.tmp/configs',
                dest: '<%= yeoman.dist %>/configs',
                src: ['*']
            }]
        },
        styles: {
            files: [{
                expand: true,
                cwd: '.tmp/styles',
                dest: '<%= yeoman.dist %>/styles',
                src: ['*']
            }]
        },
        images: {
            files: [{
                expand: true,
                cwd: '.tmp/images',
                dest: '<%= yeoman.dist %>/images',
                src: ['generated/*']
            }]
        },
        fonts: {
            expand: true,
            flatten: true,
            cwd: '<%= yeoman.app %>',
            dest: '<%= yeoman.dist %>/styles/fonts',
            src: '**/origin-ui-toolkit/fonts/{,*/}*.{eot,svg,ttf,woff}'
        },
        docs: {
            files: [{
                expand: true,
                cwd: '<%= yeoman.dist %>/bower_components/origin-components/dist',
                src: 'directives/**',
                dest: 'docs/dist'
            }, {
                expand: true,
                cwd: '<%= yeoman.dist %>/bower_components/origin-components/dist',
                src: 'images/**',
                dest: 'docs/dist'
            }, {
                expand: true,
                cwd: '<%= yeoman.dist %>/bower_components/origin-ui-toolkit',
                src: 'fonts/**',
                dest: 'docs/css'
            }, {
                expand: true,
                cwd: 'ngdocs/templates',
                src: '*.html',
                dest: 'docs'
            }, {
                dest: '<%= yeoman.dist %>/',
                src: 'docs/**'
            }]
        }

    };
};