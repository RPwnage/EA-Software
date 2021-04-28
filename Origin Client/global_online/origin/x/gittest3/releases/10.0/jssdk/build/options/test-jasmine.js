module.exports = function() {
    return {
        test: {
            src: [
                'dist/origin-jssdk.js'
            ],
            options: {
                vendor: [
                    'test/helper/helper.js'
                ],
                specs: [
                    'test/spec/initSpec.js',
                    'test/spec/authSpec.js',
                    'test/spec/gamesSpec.js',
                    'test/spec/xmppSpec.js',
                    'test/spec/atomSpec.js',
                    'test/spec/avatarSpec.js'
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
