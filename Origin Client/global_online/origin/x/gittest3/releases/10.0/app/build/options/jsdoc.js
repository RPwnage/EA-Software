module.exports = function() {
    return {
        dist: {
            src: ['src/**/*.js', 'src/*.js', '!src/bower_components/**/*.js'],
            options: {
                destination: 'docs_gen'
            }
        }
    };
};