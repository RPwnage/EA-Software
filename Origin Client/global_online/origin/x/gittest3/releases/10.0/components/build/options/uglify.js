module.exports = function() {
    return {
	    dist: {
	        files: {
	            '<%= yeoman.dist %>/scripts/origincomponents.min.js': [
	                '<%= yeoman.dist %>/scripts/origincomponents.js'
	            ]
	        }
	    }
	};
};