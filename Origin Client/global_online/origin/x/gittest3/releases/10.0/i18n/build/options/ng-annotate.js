module.exports = function() {
	return {
        dist: {
            files: [{
                expand: true,
                cwd: '<%= yeoman.dist %>',
                src: '*.js',
                dest: '<%= yeoman.dist %>'
            }]
        }
    };
};