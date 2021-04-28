'use strict';
var grunt = require('grunt');

exports.templatecompile = {
  compiledirectivetemplate1: function (test) {
    test.expect(1);

    var actual = grunt.file.read('tmp/fixtures/directives/scripts/directive-example.js');
    var expected = grunt.file.read('test/expected/directive-example.js');
    test.equal(actual, expected, 'Should handle two directives in the same file with different reference locations. Also encodes and minifies HTML into a javascript compatible string.');

    test.done();
  },
  compiledirectivetemplate2: function (test) {
    test.expect(1);

    var actual = grunt.file.read('tmp/fixtures/directives/scripts/directive-2-example.js');
    var expected = grunt.file.read('test/expected/directive-2-example.js');
    test.equal(actual, expected, 'Simple Case of one directive in a file. Should replace templateUrl with template');

    test.done();
  },
  templateUrlProp: function (test) {
    test.expect(1);

    test.ok(!grunt.file.exists('tmp/fixtures/directives/scripts/templateUrlUtil.js'), 'JavaScript files without Angular JS directive code should not be compiled by this tool to lower the risk surface of interpreting all js files with the string templateUrl: "" in it');

    test.done();
  },
  randomCode: function (test) {
    test.expect(1);

    test.ok(!grunt.file.exists('tmp/fixtures/directives/scripts/nomatches.js'), 'AngularJS files with no matching directive templateUrls should not be modified. Moving these files can be done with a copy tool in a previous step instead to save processing time.');

    test.done();
  }
};