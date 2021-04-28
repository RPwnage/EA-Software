
module.exports = function() {
    return {
        compileCore: {
            options: {
                strictMath: true,
                sourceMap: true,
                outputSourceFiles: true,
                sourceMapURL: 'origincomponents.css.map',
                sourceMapFilename: '.tmp/styles/origincomponents.css.map'
            },
            files: {
                '.tmp/styles/origincomponents.css': 'src/less/origincomponents.less'
            }
        }
    };
};