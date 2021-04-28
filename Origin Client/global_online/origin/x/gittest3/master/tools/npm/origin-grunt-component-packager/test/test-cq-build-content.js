'use strict';
var fs = require('fs'),
    contentBuilder = require('../tasks/lib/cq-build-content.js')('utf-8');

exports.testComponentContentBuild = {
    build: function (test) {
        test.expect(1);

        var actual = contentBuilder.buildDirectiveContentXml('HomeRecommendedActionTrial', 'Web: game', '/apps/originx/components/imported/dialogs/origin-home-recommended-trial');
        var expected = fs.readFileSync('test/expected/.content.xml').toString();
        test.equal(actual, expected, 'Content XML file is properly formatted');

        test.done();
    }
};
