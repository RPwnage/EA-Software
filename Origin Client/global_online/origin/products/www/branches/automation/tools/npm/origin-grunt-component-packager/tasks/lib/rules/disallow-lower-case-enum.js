/**
 * Disallows lower case enumeration


**/

var assert = require('assert');
var helper = require('./helper.js')();

module.exports = function() {};


module.exports.prototype = {
  configure: function(options) {
    assert(
      options === true,
      this.getOptionName() + ' option requires a true value or should be removed'
    );
  },

  getOptionName: function() {
    return 'disallowLowerCaseEnumeration';
  },

  check: function(file, errors) {
  }
};
