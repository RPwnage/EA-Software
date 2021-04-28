;(function( $, undefined ) {

    "use strict";

    $.fn.containerScrollbar = function( options, parameters ) {

        var settings = {
            navigationHeight : 0,
            wrapper : null
        };

        var methods = {
                
            init : function() {

                $.extend( settings, options );

                methods.helpers.setContainerHeight();

                onlineStatus.onlineStateChanged.connect( function(state){
                    methods.helpers.setContainerHeight();
                });

                $(window).on("resize", function(){
                    methods.helpers.setContainerHeight();
                });


            }, //END init

            helpers : {
                setContainerHeight : function(){

                    settings.navigationHeight = $(".nav-wrapper").height() +
                                                    parseInt($(".nav-wrapper").css("padding-top")) +
                                                    parseInt($(".nav-wrapper").css("padding-bottom"));


                     settings.wrapper = $(".outer-wrapper > div.wrapper").height( $(window).height() - settings.navigationHeight);

                     settings.wrapper.height( $(window).height() - settings.navigationHeight);

                }
            }

        };// END: methods

        // initialization
        
        var publicMethods = [ "init" ];     
        
        if ( typeof options === 'object' || ! options ) {

            methods.init(); // initializes plugin

        } else if ( $.inArray( options, publicMethods ) ) {
            // call specific methods with arguments
            
        };

    };  // END: $.containerScrollbar = function() {


})( jQuery ); // END: (function( $ ){


$(document).ready(function() {
    $.fn.containerScrollbar({});
});