;(function($){
"use strict";

// Returns a function that keeps calling the property called iterProp until it matches the passed selector
function matchSiblingFunction(iterProp)
{
    return function(selector)
    {
        for(var $el = this[iterProp]();
            $el.length !== 0;
            $el = $el[iterProp]())
        {
            if ($el.is(selector))
            {
                return $el;
            }
        }

        // Nothing was found
        return $();
    }
}

// Return the first preceding sibling matching the passed selector 
$.fn.prevMatching = matchSiblingFunction('prev');
// Return the first succeeding sibling matching the passed selector
$.fn.nextMatching = matchSiblingFunction('next');

})(jQuery);
