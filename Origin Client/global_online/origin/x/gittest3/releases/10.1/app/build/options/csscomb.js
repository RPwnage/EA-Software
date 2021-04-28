module.exports = function() {
    return {
        options: {
            config: '<%= yeoman.app %>/less/.csscomb.json'
        },
        dist: {
            files: {
                '.tmp/styles/originx.css': '.tmp/styles/originx.css'
            }
        }
    };
};