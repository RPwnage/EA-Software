module.exports = function() {
    function optionsBlock(isPrivate) {
        return {
            src: ['src/**/!(intro|outro|const)*.js'],
            options: {
                destination: 'docs_gen',
                configure: './jsdoc/jsdoc.conf.json',
                template: './jsdoc/template',
                private: isPrivate
            }
        };
    }

    return {
        dist: optionsBlock(false),
        internal: optionsBlock(true)
    };
};
