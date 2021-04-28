module.exports = function() {
    function options(minificationMethod) {
        return {
            options: {
                baseUrl: "src/",
                optimize: minificationMethod,
                out: "dist/eax-experimentsdk.js",
                name: "../bower_components/almond/almond",
                include: ["experimentsdk"],
                wrap: {
                    startFile: "build/wrapper/intro.js",
                    endFile: "build/wrapper/outro.js"
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
