// Compile html templates into directive files for use with e2e testing

'use strict';

module.exports = function() {
    return {
        options: {
            removeComments: true,
            collapseWhitespace: true
        },
        compile: {
            files: [{
                cwd: 'src/directives',
                dest: '.tmp/tc_directives',
                src: [
                    '**/*.js'
                ]
            }]
        }
    };
};