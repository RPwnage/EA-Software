module.exports = function() {
    return {
        main: {
            files: [{
                expand: true,
                cwd: 'dist/',
                src: ['**/*.js'],
                dest: 'jsdoc/template/static/scripts/'
            }, {
                expand: true,
                cwd: 'bower_components/',
                src: ['**/*.js'],
                dest: 'jsdoc/template/static/scripts/'
            }]
        },
        post: {
            files: [{
                src: 'docs_gen/index.html',
                dest: 'docs_gen/home.html'
            }, {
                src: 'docs_gen/index.entry.html',
                dest: 'docs_gen/index.html'
            }]
        },
        docstoappdist: {
            files: [{
                   expand: true,
                cwd: 'docs_gen/',
                src: '**',
                dest: '../app/dist/jssdk-docs/'
            }]
        }
    };
};
