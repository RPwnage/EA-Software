module.exports = function() {
    return {
        options: {
            dest: 'docs',
            scripts: [
                '.tmp/docs/docsdepends.js',
                'ngdocs/scripts/init.js'
            ],
            styles: [
                'dist/bower_components/origin-components/dist/styles/origincomponents.css',
                'dist/bower_components/origin-ui-toolkit/dist/otk.css',
                'ngdocs/styles/override.css'
            ],
            html5Mode: false,
            title: 'Origin X SPA docs',
            bestMatch: true,
            editExample: false,
            navTemplate: 'ngdocs/templates/mynav.html'
        },
        all: {
            src: [
                'src/routing/**/*.js',
                'src/controllers/**/*.js',
                'src/factories/**/*.js',
                'src/bootstrap/**/*.js',
                'src/*.js',
                'dist/bower_components/origin-components/src/**/*.js'
            ]
        }
    };
};