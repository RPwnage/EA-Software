;(function(){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }

// Returns a function that will call the fn parameter only once per second, regardless of how many times the throttle function is called
Origin.utilities.throttle = function(fn, delay) 
{
  delay || (delay = 1000);
  var prev,
      deferTimer;
  return function () 
  {
    var context = this;
    var now = +new Date(),
        args = arguments;
    if (prev && now < (prev + delay)) 
    {
      clearTimeout(deferTimer);
      deferTimer = setTimeout(function () 
      {
        prev = now;
        fn.apply(context, args);
      }, delay);
    } else {
      prev = now;
      fn.apply(context, args);
    }
  };
}


// Returns a function that will call the fn parameter only once per second and eats any subsequent calls until the one second is ended
Origin.utilities.drop_throttle = function(fn, delay) 
{
  delay || (delay = 1000);
  var prev,
      throttleTimer;
  return function () 
  {
    var context = this;
    var args = arguments;
    if (!throttleTimer) 
    {
      fn.apply(context, args);
      
      throttleTimer = setTimeout(function () 
      {
        throttleTimer = null;
      }, delay);
      
    }
  };
}


// This function returns a function that will call apply() on the fn parameter, after 1 second delay, providing that no other calls to the function are made. If the function is called before the delay timeout has passed, the 1 second delay is re-set.
Origin.utilities.reverse_throttle = function(fn, delay) 
{
  delay || (delay = 1000);
  var deferTimer = null;
  return function () 
  {
    var context = this;
    var args = arguments;
    if (deferTimer) {
      clearTimeout(deferTimer);
    }
    deferTimer = setTimeout(function () 
    {
        deferTimer = null;
        fn.apply(context, args);
    }, delay);
  }        
}

}());