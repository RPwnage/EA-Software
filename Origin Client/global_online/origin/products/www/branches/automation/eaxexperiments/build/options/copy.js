module.exports = function() {
	return {
        dist: {
            src: [
				'.tmp/constants/{,*/}*.js',
                '<%= yeoman.app %>/eax-experiments.js',
                '<%= yeoman.app %>/{,*/}*.js',
                '<%= yeoman.app %>/directives/scripts{,*/}*.js'
            ],
            dest: '<%= yeoman.dist %>/scripts/eax-experiments.js'
        },
        distassets: {
            files: [
                {
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
                }
            ]
        }
    };
};
