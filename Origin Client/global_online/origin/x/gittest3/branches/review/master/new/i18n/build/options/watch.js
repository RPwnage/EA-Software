module.exports = function() {
    return {
        js: {
            files: [
                '<%= yeoman.app %>/{,*/}*.js'
            ],
            tasks: ['build'],
            options: {
                livereload: 35728
            }
        },
        gruntfile: {
            files: ['Gruntfile.js']
        },
        livereload: {
            options: {
                livereload: 35728
            },
            files: []
        }
    };
};