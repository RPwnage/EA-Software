module.exports = function() {
    return {
        options: {
            csslintrc: '<%= yeoman.app %>/less/.csslintrc'
        },
        src: ['.tmp/styles/origincomponents.css']
    };
};