module.exports = function() {
    return {
        unit: {
            configFile: 'karma.conf.js',
            singleRun: true
        },
        slim: {
            configFile: 'karma-slim.conf.js',
            singleRun: true
        }
    };
};