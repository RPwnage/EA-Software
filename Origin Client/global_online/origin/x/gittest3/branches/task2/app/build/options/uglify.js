module.exports = function() {
    return {
        options: {
            compress: {
                'global_defs': {
                    'DEBUG': false
                },
                'dead_code': true
            }
        },
    };
};