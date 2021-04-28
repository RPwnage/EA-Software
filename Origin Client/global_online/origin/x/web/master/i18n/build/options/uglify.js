module.exports = function() {
	return {
        dist: {
            files: {
                '<%= yeoman.dist %>/origin-i18n.min.js': [
                    '<%= yeoman.dist %>/origin-i18n.js'
                ]
            }
        }
    };
};