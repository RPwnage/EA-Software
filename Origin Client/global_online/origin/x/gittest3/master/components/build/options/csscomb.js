module.exports = function() {
    return {
        options: {
            config: '<%= yeoman.app %>/less/.csscomb.json'
        },
        dist: {
            files: {
                '.tmp/styles/origincomponents.css': '.tmp/styles/origincomponents.css'
            }
        }
    };
};