$(function() {
    "use strict";

    if (!window.Origin) { window.Origin = {}; }
    if (!Origin.views) { Origin.views = {}; }
    if (!Origin.model) { Origin.model = {}; }
    if (!Origin.util) { Origin.util = {}; }
    
    // Add our platform attribute for the benefit of CSS
    $(document.body).attr('data-platform', Origin.views.currentPlatform());

    $('#loginForm').submit(function(event) {
        $('#general-auth').hide();
        $(document.body).addClass('wait');
        var query = shiftQuery.authenticate($('#email').val(), $('#password').val());

        query.fail.connect(function() {
            $('#general-auth').show();
        });

        query.always.connect(function() {
            $(document.body).removeClass('wait');
        });
    });

    $('#btn-login').on('click', function(event) {
        $('#loginForm').submit();
    });

    $('#btn-cancel').on('click', function(event){
        shiftQuery.closeLoginWindow();
    });
    
    $('#email').keypress(function(event) {
        if (event.which == 13) {
            $('#loginForm').submit();
            return false;
        }
    });
    
    $('#password').keypress(function(event) {
        if (event.which == 13) {
            $('#loginForm').submit();
            return false;
        }
    });
});
