// By default, your `index.html`'s <!-- Usemin block --> will take care of
// minification. These next options are pre-configured if you do not wish
// to use the Usemin blocks.

module.exports = function() {
    return {
        dist: {
            files: {
                '.tmp/styles/origincomponents.min.css': [
                    '.tmp/styles/origincomponents.css'
                ]
            }
        }
    };
};