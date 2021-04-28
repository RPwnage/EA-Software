module.exports = function() {
	return {
        dist: {
            files: {
                '<%= yeoman.dist %>/scripts/eax-experiments.min.js': [
                    '<%= yeoman.dist %>/scripts/eax-experiments.js'
                ]
            }
        }
    };
};