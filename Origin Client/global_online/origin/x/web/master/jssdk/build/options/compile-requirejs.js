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
                    'blob': '../bower_components/blob-polyfill/Blob',
                    'strophe': '../bower_components/strophe/strophe',
                    'stacktrace': '../bower_components/stacktrace-js/stacktrace',
                    'xml2json': 'core/xml2json',
                    'QWebChannel':'webchannel/qwebchannel'
                },
                shim: {
                    'uuid': {
                        exports: 'uuid'
                    },
                    'promise': {
                        exports: 'Promise'
                    },
                    'blob': {
                        exports: 'Blob'
                    },
                    'strophe': {
                        exports: 'Strophe'
                    },
                    'xml2json': {
                        exports: 'xml2json'
                    },
                    'patches/strophe-patch.js': {
                        deps: ['strophe']
                    },
                    'QWebChannel': {
                        exports: 'QWebChannel'
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
