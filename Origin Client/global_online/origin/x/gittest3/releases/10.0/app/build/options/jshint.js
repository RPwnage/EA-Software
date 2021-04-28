// Make sure code styles are up to par and there are no obvious mistakes

module.exports = function() {
    return {
	    options: {
	        jshintrc: '.jshintrc',
	        reporter: require('jshint-stylish')
	    },
	    all: {
	        src: [
	            'Gruntfile.js',
	            'src/routing/**/*.js',
	            'src/controllers/**/*.js',
	            'src/factories/**/*.js',
	            'src/bootstrap/**/*.js',
	            'src/*.js'
	        ]
	    },
	    test: {
	        options: {
	            jshintrc: 'test/.jshintrc'
	        },
	        src: ['test/spec/{,*/}*.js']
	    }
	};
};