'use strict';
var fs = require('fs'),
    jspBuilder = require('../tasks/lib/cq-build-jsp.js')();

exports.testJspBuild = {
    buildDefaultUi: function (test) {
        test.expect(1);

        var source = JSON.parse(fs.readFileSync('test/expected/source-extractor-extract.json'));
        var template = fs.readFileSync('tasks/templates/default.jsp.tpl').toString();
        var actual = jspBuilder.buildDefaultUi(template, 'origin-home-recommended-action-trial', source.params);
        var expected = fs.readFileSync('test/expected/default.jsp').toString();
        test.equal(actual, expected, 'Default JSP generated');

        test.done();
    },

    buildMainUi: function (test) {
        test.expect(1);

        var source = JSON.parse(fs.readFileSync('test/expected/source-extractor-extract.json'));
        var template = fs.readFileSync('tasks/templates/main.jsp.tpl').toString();
        var actual = jspBuilder.buildMain(template,
            '/apps/originx/components/imported/directives/scripts/origin-home-recommended-action-trial/origin-home-recommended-action-trial.default.jsp',
            '/apps/originx/components/imported/directives/scripts/origin-home-recommended-action-trial/origin-home-recommended-action-trial.override.jsp',
            source.params,
            source.info.directiveName);
        var expected = fs.readFileSync('test/expected/main.jsp').toString();
        test.equal(actual, expected, 'Main JSP generated');

        test.done();
    }
};
