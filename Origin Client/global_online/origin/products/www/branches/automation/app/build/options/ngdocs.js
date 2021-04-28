module.exports = function() {
    return {
        options: {
            dest: 'docs',
            scripts: [
                'dist/bower_components/angular/angular.js',
                'dist/bower_components/angular-animate/angular-animate.js',
                'dist/bower_components/origin-locale/dist/origin-locale.js',
                'dist/bower_components/lodash/lodash.js',
                'dist/bower_components/jquery/dist/jquery.js',
                'dist/bower_components/angular-touch/angular-touch.js',
                'dist/bower_components/angular-ui-router/release/angular-ui-router.js',
                'dist/bower_components/angular-toArrayFilter/toArrayFilter.js',
                'dist/bower_components/angular-resource/angular-resource.js',
                'dist/bower_components/angular-cookies/angular-cookies.js',
                'dist/bower_components/angular-sanitize/angular-sanitize.js',
                'dist/bower_components/angular-route/angular-route.js',
                'dist/bower_components/moment/min/moment-with-locales.min.js',
                'dist/bower_components/origin-jssdk/dist/origin-jssdk.js',
                'dist/bower_components/origin-i18n/dist/origin-i18n.js',
                'dist/bower_components/uri.js/src/URI.js',
                'dist/bower_components/origin-components/dist/scripts/origincomponents.js',
                'ngdocs/scripts/init.js'
            ],
            styles: [
                'dist/bower_components/origin-components/dist/styles/origincomponents.css',
                'dist/bower_components/otk/dist/otk.css'
            ],
            html5Mode: false,
            title: 'Origin X Directives',
            bestMatch: true,
            editExample: false,
            navTemplate: 'ngdocs/templates/mynav.html'
        },
        all: {
            src: [
                'dist/bower_components/origin-components/src/directives/**/*.js'
            ]
        }
    };
};
