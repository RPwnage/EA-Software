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
                    'xml2json': 'core/xml2json',
                    'QWebChannel':'webchannel/qwebchannel',
                    'usertiming': '../bower_components/usertiming/src/usertiming'
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
                    },
                    'usertiming' : {
                        exports: 'window.performance'
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

    function noLogOptions() {
        var optionsObj = options("uglify");
        optionsObj.options.uglify.mangle= false;
        optionsObj.options.uglify.beautify = true;
        return optionsObj;
    }

    return {
        dev: options("none"),
        minified: options("uglify"),
        nologs: noLogOptions()
    };
};
