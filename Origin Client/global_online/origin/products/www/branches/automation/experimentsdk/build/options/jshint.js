module.exports = function() {
    return {
        all_files: [
            'grunt.js',
            'src/**/!(intro|outro|const)*.js',
            '!src/**/xml2json.js'
        ],
        options: {
            jshintrc: '.jshintrc'
        }
    };
};
