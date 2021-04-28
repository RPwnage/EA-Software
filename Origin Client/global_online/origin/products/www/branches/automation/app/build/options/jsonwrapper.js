module.exports = function() {
    return {
        options: {
            wrapper: '(function(){if (typeof(OriginKernel) === \'undefined\') OriginKernel = {};if (typeof(OriginKernel.configs) === \'undefined\') OriginKernel.configs = {};OriginKernel.configs[\'{filePath}\'] = \{content}\;}());',
        },
        mixed: {
            files: {
                'dist/configs.js' : ['dist/configs/*.json', 'dist/configs/environment/production/*.json']
            }
        }
    };
};
