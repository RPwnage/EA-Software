/**
 * Disallows some Enumeration in Origin X
 * there are all duplicated definish of BooleanEnumeration

**/
var assert = require('assert');

var bannedList = require('./banned-enumerations.js');
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
    return 'disallowBannedEnumeration';
  },

  check: function(file, errors) {
  }
};
