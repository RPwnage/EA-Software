;(function( $, undefined ) {

    "use strict";

     // I don't understand why there are left over bits from HAL that require this to be done.
     // It's possible I'm doing this unnecessarily, but from everyone I've talked to this seems like a requirement still.

    $.fn.localization = function( options, parameters  ) {

        var settings = {
            stringReplacements : {
                "ebisu_client_offline_mode_message_text" : "Origin"
            }
        };

        // bindings
        var methods = {
                
            init : function() {

                $.extend( settings, options );

                var $translated =  $("*[data-string-key]");
                
                $.each( $translated, function( index, translatedElement ){

                    if ( $(translatedElement).text().indexOf("%1") > -1 ){
                        var key = $(translatedElement).attr("data-string-key");
                        if ( settings.stringReplacements[key] != null ){
                            $(translatedElement).text( $(translatedElement).text().replace( "%1", settings.stringReplacements[key]) );
                        };
                    }

                });

            }, //END init

        };// END: methods

 
        // initialization
        
        var publicMethods = [ "init" ];     
        
        if ( typeof options === 'object' || ! options ) {

            methods.init(); // initializes plugin

        } else if ( $.inArray( options, publicMethods ) ) {
            // call specific methods with arguments
            
        };



    };  // END: $.localization = function() {
        


})( jQuery ); // END: (function( $ ){


$(document).ready(function() {
    $.fn.localization({});
});