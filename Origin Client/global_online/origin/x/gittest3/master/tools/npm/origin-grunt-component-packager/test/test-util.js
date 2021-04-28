'use strict';
var util = require('../tasks/lib/util')();

exports.testUtils = {
    toManlyCaseEmpty: function (test) {
        test.expect(1);

        var actual = util.toManlyCase('');
        var expected = '';
        test.equal(actual, expected, 'Empty String');

        test.done();
    },

    toManlyCaseNoHyphen: function (test) {
        test.expect(1);

        var actual = util.toManlyCase('foo');
        var expected = 'Foo';
        test.equal(actual, expected, 'Upper case first word');

        test.done();
    },

    toManlyCaseHyphen: function (test) {
        test.expect(1);

        var actual = util.toManlyCase('foo-bar-baz');
        var expected = 'FooBarBaz';
        test.equal(actual, expected, 'Upper each hyphenated word');

        test.done();
    },

    getCrxNodeNameEmpty: function (test) {
        test.expect(1);

        var actual = util.getCrxNodeName('', '', 'origin');
        var expected = '';

        test.equal(actual, expected, 'Empty Crx Name');

        test.done();
    },

    getCrxNodeNameRegularUse: function (test) {
        test.expect(1);

        var actual = util.getCrxNodeName('origin-home-recommended-next-action-trial', 'home', 'origin');
        var expected = 'recommended-next-action-trial';

        test.equal(actual, expected, 'Removes origin-home- from string');

        test.done();
    },

    getCrxNodeNameNoMatch: function (test) {
        test.expect(1);

        var actual = util.getCrxNodeName('origin-dialog', '', 'origin');
        var expected = 'origin-dialog';

        test.equal(actual, expected, 'Name is left as is');

        test.done();
    },

    getCrxNodeNameMatchTooShort: function (test) {
        test.expect(1);

        var actual = util.getCrxNodeName('origin-dialog', 'dialog', 'origin');
        var expected = 'origin-dialog';

        test.equal(actual, expected, 'Name is left as is');

        test.done();
    },

    getDirectiveGroupNameEmpty: function (test) {
        test.expect(1);

        var actual = util.getDirectiveGroupName('');
        var expected = '';

        test.equal(actual, expected, 'Empty groupname successful');

        test.done();
    },

    getDirectiveGroupNameRegularUse: function (test) {
        test.expect(1);

        var actual = util.getDirectiveGroupName('home/recommended/action/scripts/trial.js');
        var expected = 'home';

        test.equal(actual, expected, 'Extract groupname');

        test.done();
    },

    getDirectiveGroupNameTooShort: function (test) {
        test.expect(1);

        var actual = util.getDirectiveGroupName('trial.js');
        var expected = 'trial';

        test.equal(actual, expected, 'Extract groupname');

        test.done();
    },

    endsWithTrue: function(test) {
        test.expect(1);

        var actual = util.endsWith('ButtonTypeEnumeration', "Enumeration");
        var expected = true;

        test.equal(actual, expected);

        test.done();

    },

    endsWithFalse: function(test) {
        test.expect(1);

        var actual = util.endsWith('ButtonTypeEnumeration', "Type");
        var expected = false;

        test.equal(actual, expected);

        test.done();
    }
};
