// Allow the use of non-minsafe AngularJS files. Automatically makes it
// minsafe compatible so Uglify does not destroy the ng references

module.exports = function() {
    return {
        dist: {
            files: [{
                expand: true,
                cwd: '<%= yeoman.dist %>/scripts',
                src: '*.js',
                dest: '<%= yeoman.dist %>/scripts'
            }]
        }
    };
};