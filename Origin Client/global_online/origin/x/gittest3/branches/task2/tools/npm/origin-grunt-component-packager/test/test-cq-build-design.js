'use strict';
var fs = require('fs'),
    designBuilder = require('../tasks/lib/cq-build-design')();

exports.testDesignBuild = {
    init: function (test) {
        test.expect(1);

        designBuilder.init('test', 'utf-8');
        var actual = designBuilder.asXml();
        var expected = fs.readFileSync('test/expected/design-bare.xml').toString();
        test.equal(actual, expected, 'Bare design skeleton is correctly formatted');

        test.done();
    },

    emptyComponentDesign: function(test) {
        test.expect(1);

        designBuilder.init('test', 'utf-8');
        designBuilder.addComponent('my-page', []);
        var actual = designBuilder.asXml();
        var expected = fs.readFileSync('test/expected/design-emptycomponent.xml').toString();
        test.equal(actual, expected);

        test.done();
    },

    singleComponentDesign: function(test) {
        test.expect(1);

        designBuilder.init('test', 'utf-8');
        designBuilder.addComponent('my-page', ['/path/to/1']);
        var actual = designBuilder.asXml();
        var expected = fs.readFileSync('test/expected/design-singlecomponent.xml').toString();
        test.equal(actual, expected);

        test.done();
    },

    multiComponentDesign: function(test) {
        test.expect(1);

        designBuilder.init('test', 'utf-8');
        designBuilder.addComponent('my-page', ['/path/to/directive-1', '/path/to/directive-2', '/path/to/directive-3']);
        var actual = designBuilder.asXml();
        var expected = fs.readFileSync('test/expected/design-multicomponent.xml').toString();
        test.equal(actual, expected);

        test.done();
    }
};
