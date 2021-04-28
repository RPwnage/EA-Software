'use strict';
var fs = require('fs');

exports.templatecompile = {
  compilelessfile: function (test) {
    test.expect(1);

    var actual = fs.readFileSync('tmp/fixtures/less/fonts.less').toString();
    var expected = fs.readFileSync('test/expected/fonts.less').toString();
    test.equal(actual, expected, 'Handles data URI for woff and ttf');

    test.done();
  }
};