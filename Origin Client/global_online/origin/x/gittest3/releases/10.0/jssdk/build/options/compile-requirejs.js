module.exports = function() {
    function options(minificationMethod) {
        return {
            options: {
                baseUrl: "src/",
                optimize: minificationMethod,
                out: "dist/origin-jssdk.js",
                name: "../bower_components/almond/almond",
                include: ["jssdk"],
                wrap: {
                    startFile: "build/wrapper/intro.js",
                    endFile: "build/wrapper/outro.js"
                },
                paths: {
                    'uuid': '../bower_components/node-uuid/uuid',
                    'promise': '../bower_components/promise-polyfill/Promise',
                    'strophe': '../bower_components/strophe/strophe',
                    'stacktrace': '../bower_components/stacktrace-js/stacktrace',
                    'xml2json': 'core/xml2json'
                },
                shim: {
                    'uuid': {
                        exports: 'uuid'
                    },
                    'promise': {
                        exports: 'Promise'
                    },
                    'strophe': {
                        exports: 'Strophe'
                    },
                    'xml2json': {
                        exports: 'xml2json'
                    },
                    'patches/strophe-patch.js': {
                        deps: ['strophe']
                    }
                },
                uglify: {
                    defines: {
                        STRIPCONSOLE: ['name', 'true']
                    },
                    'dead_code': true
                },
                wrapShim: true,
                 useStrict: true,
                removeCombined: true
            }
        };
    }

    return {
        dev: options("none"),
        minified: options("uglify")
    };
};
