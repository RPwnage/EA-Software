/**
 * check the enmurations type defined in origin X.
**/

module.exports = function(grunt) {

    grunt.registerMultiTask('check-enumerations', 'Police Origin X enmuration types', function() {
        grunt.log.write("checking origin X enmuration types\r\n");
        grunt.log.write(__dirname + "\n");
        var Checker = require('jscs');
        var path = require('path');
        var checker = new Checker();
        checker.configure({
            "disallowLowerCaseEnumeration": true,
            "disallowBannedEnumeration": true,
            "buttonTypeCheck": true,
            "additionalRules": [__dirname + "/lib/rules/disallow*.js",
             __dirname + "/lib/rules/*Check.js"]
        });

        this.files.forEach(function(fileList) {
            fileList.src.forEach(function(file) {
                var sourceCode = grunt.file.read(path.resolve(file));
                var results = checker.checkString(sourceCode, file);
                results.getErrorList().forEach(function(error) {
                    var colorizeOutput = true;
                    grunt.log.write(results.explainError(error, colorizeOutput) + "\n");
                });
            });
        });

    });
};
