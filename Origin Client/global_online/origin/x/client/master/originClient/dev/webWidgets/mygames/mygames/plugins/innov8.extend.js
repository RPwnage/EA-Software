/**
 * Created by Innov8 Interactive, LLC
 * User: mlilli
 * Date: Mar 13, 2012
 */

Function.prototype.delegate = function(scope)
{
    if (!scope) { scope = this; }
    var fn = this;
    return function() { return fn.apply(scope, arguments); };
};

Function.prototype.curry = function(scope)
{
    if (!scope) { scope = this; }
    var fn = this;
    var args = Array.prototype.slice.call(arguments, 1);
    return function() { return fn.apply(scope, $.makeArray(arguments).concat(args)); };
};