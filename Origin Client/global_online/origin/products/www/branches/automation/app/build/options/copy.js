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
                    'images/*',
                    'age-de.xml',
                    'kernel.js',
                    'perf-constants.js',
                    'staticshell.js',
                    'basehref.js'
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
                    'age-de.xml',
                    'kernel.js',
                    'perf-constants.js',
                    'staticshell.js',
                    'basehref.js'
                ]
            }]
        },
        componentimages: {
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= yeoman.dist %>/bower_components/origin-components/dist/',
                dest: '<%= yeoman.dist %>',
                src: [
                    'images/**'
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
        bowerEAX: {
            files: [{
                expand: true,
                dot: true,
                cwd: '<%= yeoman.app %>',
                dest: '<%= yeoman.dist %>',
                src: [
                    'bower_components/eax-*/dist/**'
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
                    'directives/**/*.js',
                    'factories/**/*.js',
                    'preload/**/*.js',
                    'workers/*.js',
                    'telemetry/**/*.js',
                    '*.js'
                ]
            }]
        },
        preload: {
            files: [{
                expand: true,
                cwd: 'preload/src',
                dest: '<%= yeoman.dist %>',
                src: ['*']
            }]
        },
        ngconstants: {
            files: [{
                expand: true,
                cwd: '.tmp/constants',
                dest: '<%= yeoman.dist %>/constants',
                src: ['*']
            }]
        },
        configs: {
            files: [{
                expand: true,
                dot: true,
                cwd: '../tools/config',
                dest: '<%= yeoman.dist %>/configs',
                src: [
                    '**/*.json'
                ]
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
        icons: {
            files: [{
                expand: true,
                cwd: 'src/images',
                dest: '<%= yeoman.dist %>',
                src: ['favicon.ico']
            }]
        },
        fonts: {
            expand: true,
            flatten: true,
            cwd: '<%= yeoman.dist %>/bower_components/otk/dist',
            dest: '<%= yeoman.dist %>/styles/fonts',
            src: 'fonts/{,*/}*.{eot,svg,ttf,woff}'
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
