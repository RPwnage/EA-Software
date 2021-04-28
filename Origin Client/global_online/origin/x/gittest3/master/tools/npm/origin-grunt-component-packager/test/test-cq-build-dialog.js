'use strict';
var fs = require('fs'),
    dialogBuilder = require('../tasks/lib/cq-build-dialog.js')('utf-8');

exports.testContentBuild = {
    build: function (test) {
        test.expect(1);

        var source = JSON.parse(fs.readFileSync('test/expected/source-extractor-extract.json'));
        var actual = dialogBuilder.build(source);
        var expected = fs.readFileSync('test/expected/dialog.xml').toString();
        test.equal(actual, expected, 'Content XML file is properly formatted');

        test.done();
    }
};
