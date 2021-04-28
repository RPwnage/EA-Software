/*
 * Query String 1.0
 * (c) 2012 INNOV8 Interactive, LLC <innov8ia.com>
 * MIT license
 *
 */

;(function($){ 

$.getQueryString = function(name)
{
    name = name.replace(/[\[]/, '\\\[').replace(/[\]]/, '\\\]');
    var regex = new RegExp('[\\?&]' + name + '=([^&#]*)');
    var results = regex.exec(window.location.search);
    return (results == null)? '' : decodeURIComponent(results[1].replace(/\+/g, ' '));
};

})(jQuery);