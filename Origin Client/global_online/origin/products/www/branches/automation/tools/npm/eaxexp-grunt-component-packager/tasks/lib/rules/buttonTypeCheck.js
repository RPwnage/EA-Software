/**
 * Check the ButtonType definations

**/
var assert = require('assert');

var buttonTypes = require('./buttonTypes.js')();
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
    return 'buttonTypeCheck';
  },

  check: function(file, errors) {
  }
};
