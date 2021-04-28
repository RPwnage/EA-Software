'use strict';
var fs = require('fs'),
    sourceExtractor = require('../tasks/lib/source-extractor.js')();

exports.testSourceExtractor = {
    extract: function (test) {
        test.expect(1);

        var sourceCode = fs.readFileSync('test/fixtures/directives/scripts/example-directive.js').toString();
        var actual = JSON.stringify(sourceExtractor.extract(sourceCode));
        var expected = JSON.stringify(JSON.parse(fs.readFileSync('test/expected/source-extractor-extract.json').toString()));
        test.equal(actual, expected, 'Source parser extracts documentation from prototype reference correctly');

        test.done();
    },

    extractNameOnly: function (test) {
        test.expect(1);

        var sourceCode = fs.readFileSync('test/fixtures/directives/scripts/name-only.js').toString();
        try{
            sourceExtractor.extract(sourceCode);
            test.ok(false, "Throws an exception when using a Name Only style docblock");
        } catch(e) {
            test.ok(true, "Throws an exception when using a Name Only style docblock");
        }

        test.done();
    },

    extractWildcardType: function (test) {
        test.expect(1);

        var sourceCode = fs.readFileSync('test/fixtures/directives/scripts/multiple-types.js').toString();
        try{
            sourceExtractor.extract(sourceCode);
            test.ok(false, "Throws an exception when using a wildcard type style docblock");
        } catch(e) {
            test.ok(true, "Throws an exception when using a wildcard type style docblock");
        }

        test.done();
    },

    extractRepeatableType: function (test) {
        test.expect(1);

        var sourceCode = fs.readFileSync('test/fixtures/directives/scripts/repeated-parameters.js').toString();
        try{
            sourceExtractor.extract(sourceCode);
            test.ok(false, "Throws an exception when using a repeatable param style docblock");
        } catch(e) {
            test.ok(true, "Throws an exception when using a repeatable param style docblock");
        }

        test.done();
    },

    extractParameterProperties: function (test) {
        test.expect(1);

        var sourceCode = fs.readFileSync('test/fixtures/directives/scripts/parameter-properties.js').toString();
        try{
            sourceExtractor.extract(sourceCode);
            test.ok(false, "Throws an exception when using a parameter properties");
        } catch(e) {
            test.ok(true, "Throws an exception when using a parameter properties");
        }

        test.done();
    },

    extractParameterPropertiesList: function (test) {
        test.expect(1);

        var sourceCode = fs.readFileSync('test/fixtures/directives/scripts/parameter-properties-list.js').toString();
        try{
            sourceExtractor.extract(sourceCode);
            test.ok(false, "Throws an exception when using a parameter properties list");
        } catch(e) {
            test.ok(true, "Throws an exception when using a parameter properties list");
        }

        test.done();
    },
};
