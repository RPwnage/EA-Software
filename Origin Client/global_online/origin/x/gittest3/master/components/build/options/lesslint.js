
module.exports = function() {
    return {
        src: ['src/less/origincomponents.less'],
        options: {
            imports: ['src/**/*.less'],
            csslintrc: '<%= yeoman.app %>/less/.csslintrc'
        }
    };
};