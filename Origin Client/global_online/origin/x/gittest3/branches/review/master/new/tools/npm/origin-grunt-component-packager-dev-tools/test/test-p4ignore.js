'use strict';

var fs = require('fs');

exports.testp4ignore = {
    p4ignore: function (test) {
        test.expect(1);

        var actual = fs.readFileSync('tmp/cq5/apps/originx/components/imported/directives/ocd/ocd/.p4ignore').toString();
        var expected = fs.readFileSync('test/expected/.p4ignore').toString();
        test.equal(actual, expected, 'P4ignore matches expected value');

        test.done();
    }
};
