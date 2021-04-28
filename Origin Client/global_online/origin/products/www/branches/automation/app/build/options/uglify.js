module.exports = function() {
    return {
        options: {
            compress: {
                'global_defs': {
                    'DEBUG': false,
                    'ANGULARDEBUG': false
                },
                'dead_code': true,
                'drop_console': true
            }
        },
    };
};