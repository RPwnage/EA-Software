// Compile html templates into directive files for use with dist builds

'use strict';

module.exports = function() {
    return {
        app: {
            cwd: 'src/directives',
            src: '**/*.html',
            dest: '.tmp/templates.js',
            options: {
                module: 'origin-components',
                quotes: 'single',
                htmlmin: {
                    collapseWhitespace: true,
                    collapseBooleanAttributes: true
                }
            }
        }
    };
};