module.exports = function() {
    return {
        test: {
            src: [
                'dist/*.js'
            ],
            options: {
                vendor: [
                    'dist/bower_components/angular/angular.js',
                    'dist/bower_components/angular-mocks/angular-mocks.js'
                ],
                specs: [
                    'test/spec/*.js'
                ],
                junit: {
                    path: 'test/',
                    consolidate: true
                },
                keepRunner: true,
                outfile: 'test/specRunner.html'
            }
        }
    };
};