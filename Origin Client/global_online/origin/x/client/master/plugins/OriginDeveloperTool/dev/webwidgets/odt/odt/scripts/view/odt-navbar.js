;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

function validateCustomFields() 
{
    // General tab has custom fields that must be validated.
    var ret = true;
    if($('#txt-geoip-custom-container').hasClass('important')) 
    {
        // Trim whitespace and make sure user has entered something.
        var ipAddress = $('#txt-geoip-custom').val().replace(/\s+/g,'');
        if(ipAddress.length === 0)
        {
            // Switch to general tab to make error obvious to user
            Origin.views.navbar.currentTab = "general";
            
            $('#txt-geoip-custom-container').addClass('field-error');
            ret = false;
        }
        else
        {
            $('#txt-geoip-custom-container').removeClass('field-error');
        }
    }
    
    if($('#txt-env-custom-container').hasClass('important')) 
    {
        // Trim whitespace and make sure user has entered something.
        var env = $('#txt-env-custom').val().replace(/\s+/g,'');
        if(env.length === 0)
        {
            // Switch to general tab to make error obvious to user
            Origin.views.navbar.currentTab = "general";
            
            $('#txt-env-custom-container').addClass('field-error');
            ret = false;
        }
        else
        {
            $('#txt-env-custom-container').removeClass('field-error');
        }
    }
    return ret;
}
 
function saveOverrides() 
{
    Origin.views.generalOverrides.setGeneralOverrides();
    if(Origin.views.navbar.currentTab == 'game-details')
    {
        Origin.views.gameDetailsOverride.setGameDetails();
        Origin.views.myGamesOverrides.init();
        Origin.views.navbar.currentTab = 'my-games';
    }
}

function doGeoQuery(callback, callRestart) 
{
    var ipAddress = '';
    if($('#txt-geoip-custom-container').hasClass('important'))
    {
        // User has entered a custom IP address.
        ipAddress = $('#txt-geoip-custom').val().replace(/\s+/g,'');
    }
    
    var geoQuery = geolocationQuery.startQuery(ipAddress);
    
    geoQuery.succeeded.connect(function(info)
    {
        console.info('Geolocation query succeeded for [' + ipAddress + ']');
        console.info('Geocountry = ' + info.commerceCountry);
        
        var locale = Origin.model.constants.DEFAULT_STORE_LOCALE;
        var $geoSelectOption = $('#div-geoip').find('select > option[country-code="'+info.geoCountry+'"]');
        if(info.geoCountry.length > 0 && $geoSelectOption.length > 0) 
        {
            locale = $geoSelectOption.attr('data-locale');
        }
        
        Origin.views.generalOverrides.customLocale = locale;
        saveOverrides(); // save first
        var result = callback();
        if (result && callRestart)
        {
            window.originDeveloper.restartClient();
        }
    });
        
    geoQuery.failed.connect(function()
    {
        // The custom IP did not resolve to a valid country, so use the default store locale.
        console.info('Geolocation query failed for ' + $('#txt-geoip-custom').val());
        Origin.views.generalOverrides.customLocale = Origin.model.constants.DEFAULT_STORE_LOCALE;
        saveOverrides(); // save first
        var result = callback();
        if (result && callRestart)
        {
            window.originDeveloper.restartClient();
        }
    });
}

var NavBarView = function()
{
    this._lastMyGamesPage = "my-games";
    this._scrollPositionMap = {};
    
    $("#navbar").find("ul li").on("click", function()
    {
        var tab = $(this).attr("data-tab");
        if(tab == "my-games")
        {
            tab = Origin.views.navbar._lastMyGamesPage;
        }
        Origin.views.navbar.currentTab = tab;
    });
    
    $("#btn-apply-changes").on("click", function()
    {
        if(validateCustomFields()) 
        {
            var $storeSelect = $('#div-store-environment').find('select.active');
            var $geoSelect = $('#div-geoip').find('select');
            if($geoSelect.val() === 'other' || ($storeSelect.val() !== '' && $geoSelect.val() === ''))
            {
                // User has entered a custom geo-IP, or no geo-IP with a non-default store environment, so we need to resolve.
                doGeoQuery(window.originCommon.applyChanges, true);
            }
            else 
            {
                Origin.views.generalOverrides.customLocale = '';
                saveOverrides(); // save first
                var restartRequired = window.originCommon.applyChanges();
                if (restartRequired)
                {
                    window.originDeveloper.restartClient();
                }
            }
        }
    });

    $("#btn-import").on("click", function()
    {
        var imported = window.originDeveloper.importFile();
        if(imported)
        {
            Origin.views.myGamesOverrides.init();
            Origin.views.generalOverrides.init();
        }
    });

    $("#btn-export").on("click", function()
    {
        if(validateCustomFields()) 
        {
            var $storeSelect = $('#div-store-environment').find('select.active');
            var $geoSelect = $('#div-geoip').find('select');
            if($storeSelect.val() !== '' && ($geoSelect.val() === 'other' || $geoSelect.val() === ''))
            {
                // User has entered a custom geo-IP, or no geo-IP with a non-default store environment, so we need to resolve.
                doGeoQuery(window.originDeveloper.exportFile, false);
            }
            else 
            {
                Origin.views.generalOverrides.customLocale = '';
                saveOverrides(); // save first
                window.originDeveloper.exportFile();
            }
        }
    });
    
    $("#origin-id").find(".value").html(window.originDeveloper.userId);
}

NavBarView.prototype.__defineGetter__('currentTab', function()
{
    return $("#main").find(".tab-content.current").attr("id");
});

NavBarView.prototype.__defineSetter__('currentTab', function(val)
{
    if (this.currentTab != val)
    {
        // Save current tab's scroll position.
        if(this.currentTab != undefined)
        {
            Origin.views.navbar._scrollPositionMap[this.currentTab] = $('#main').scrollTop();
        }
        
        // Clear and reset current content
        var tab = "";
        var content = $("#main").find(".tab-content");
        content.removeClass("current");
        content.each(function()
        {
            if($(this).attr("id") == val)
            {
                $(this).addClass("current");
                tab = $(this).attr("data-tab");
            }
        });
        
        var tabs = $("#navbar").find("ul li");
        tabs.removeClass("current");
        tabs.each(function()
        {
            if($(this).attr("data-tab") == tab)
            {
                $(this).addClass("current");
            }
        });
        
        Origin.util.resizeFixedDivs();
        
        // When navigating to game-details from my-games page, always scroll to top.  Otherwise, use persisted scroll position.
        var prevScrollPosition = Origin.views.navbar._scrollPositionMap[val];
        if((Origin.views.navbar._lastMyGamesPage == "my-games" && val == "game-details") || prevScrollPosition == undefined)
        {
            $('#main').scrollTop(0);
        }
        else
        {
            $('#main').scrollTop(prevScrollPosition);
        }
        
        if(tab == "my-games")
        {
            Origin.views.navbar._lastMyGamesPage = val;
        }
    }
});

// Expose to the world
Origin.views.navbar = new NavBarView();

})(jQuery);