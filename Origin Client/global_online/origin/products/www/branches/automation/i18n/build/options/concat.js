module.exports = function() {
	return {
        dist: {
            src: [
            	'<%= yeoman.dist %>/bower_components/angular-moment/angular-moment.js',
				'.tmp/constants/{,*/}*.js',
                '<%= yeoman.app %>/origin-i18n.js',
                '<%= yeoman.app %>/{,*/}*.js'
            ],
            dest: '<%= yeoman.dist %>/origin-i18n.js'
        }
    };
};
