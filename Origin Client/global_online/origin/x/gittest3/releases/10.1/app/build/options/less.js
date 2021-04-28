// compile our less

module.exports = function() {
    return {
        compileCore: {
            options: {
                strictMath: true,
                sourceMap: true,
                outputSourceFiles: true,
                sourceMapURL: 'originx.css.map',
                sourceMapFilename: '.tmp/styles/originx.css.map'
            },
            files: {
                '.tmp/styles/originx.css': 'src/less/originx.less'
            }
        }
    };
};