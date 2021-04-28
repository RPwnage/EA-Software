
'use strict';

module.exports = function() {
  function filterCommentByParamType(comment) {
    var regex = /@param\s*\{(.*Enumeration)\}/;
    var lines = comment.value.split("\n");
    var types = [];

    lines.forEach(function(line, index) {
      var matches = regex.exec(line);
      if (!!matches && matches.length == 2) {
        types.push({
          'type': matches[1],
          'loc':  {
            'start': {
              'line': comment.loc.start.line + index,
              'column': line.indexOf(matches[1])
            }
          }
        });
      }
    });
    return types;
  }

  function areSameObject(one, other) {
    for (var key in one) {
      if (one.hasOwnProperty(key)) {
        if (one[key] !== other[key]) {
          return false;
        }
      }
    }

    for (var key in other) {
      if(other.hasOwnProperty(key)) {
        if (one !== undefined && other[key] !== one[key]) {
          return false;
        }
      }
    }
    return true;
  }

  return {
    filterCommentByParamType: filterCommentByParamType,
    areSameObject : areSameObject
  }

};
