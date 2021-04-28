module.exports = function() {
    return {
        docs: {
            dest: '.tmp/docs/docsdepends.js',
            src: [
                'dist/bower_components/jquery/dist/jquery.js',
                'dist/bower_components/angular/angular.js',
                'dist/bower_components/angular-animate/angular-animate.js',
                'dist/bower_components/angular-resource/angular-resource.js',
                'dist/bower_components/angular-cookies/angular-cookies.js',
                'dist/bower_components/angular-sanitize/angular-sanitize.js',
                'dist/bower_components/angular-route/angular-route.js',
                'dist/bower_components/promise-polyfill/Promise.js',
                'dist/bower_components/strophe/strophe.js',
                'dist/bower_components/stacktrace-js/stacktrace.js',
                'dist/bower_components/origin-jssdk/dist/origin-jssdk.js',
                'dist/bower_components/origin-ui-toolkit/dist/otk.js',
                'dist/bower_components/ngDialog/js/ngDialog.js',
                'dist/bower_components/origin-i18n/dist/origin-i18n.js',
                'dist/bower_components/origin-components/dist/scripts/origincomponents.js'
            ]

        }
    };
};