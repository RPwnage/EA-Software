;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

var GeneralOverrideViews = function()
{
    this._customLocale = '';
}

GeneralOverrideViews.prototype.__defineGetter__('customLocale', function()
{
    return this._customLocale;
});

GeneralOverrideViews.prototype.__defineSetter__('customLocale', function(val)
{
    this._customLocale = val;
});

GeneralOverrideViews.prototype.init = function()
{
	var c = Origin.model.constants;
	var $envOptions = $('#div-environment');
	var $storeOptions = $('#div-store-environment');
    
    $('#txt-env-custom-container').removeClass('field-error');
    $('#txt-geoip-custom-container').removeClass('field-error');

    $envOptions.find('select').on('change', function(event)
    {
        var envVal = $(this).val();
		if(envVal === '')
        {
            $storeOptions.find('#store-dropdown-prod').show().find('select').addClass('active');
            $storeOptions.find('#store-dropdown-int').hide().find('select').removeClass('active');
            $storeOptions.find('#store-dropdown-other').hide().find('select').removeClass('active');
            $('#txt-env-custom-container').hide().removeClass('important');
		}
		else if(envVal === 'integration')
        {
            $storeOptions.find('#store-dropdown-int').show().find('select').addClass('active');
            $storeOptions.find('#store-dropdown-prod').hide().find('select').removeClass('active');
            $storeOptions.find('#store-dropdown-other').hide().find('select').removeClass('active');
            $('#txt-env-custom-container').hide().removeClass('important');
		}
        else
        {
            $storeOptions.find('#store-dropdown-other').show().find('select').addClass('active');
            $storeOptions.find('#store-dropdown-prod').hide().find('select').removeClass('active');
            $storeOptions.find('#store-dropdown-int').hide().find('select').removeClass('active');
            $('#txt-env-custom-container').show().addClass('important');
        }

        var $defaultOption = $storeOptions.find('select.active > option:first');
        $storeOptions.find('select.active').val($defaultOption.val());
        $storeOptions.find('.origin-ux-drop-down-selection span').text($defaultOption.text());
	});

	var environment = window.originCommon.readOverride('[Connection]', 'EnvironmentName').toLowerCase();
    if(environment == 'production')
    {
        environment = '';
    }
    
    if($envOptions.find('select > option[value="'+environment+'"]').length === 0)
    {
        $('#txt-env-custom-container').show().addClass('important');
        $('#txt-env-custom').val(environment);
        environment = 'other';
    }
    
    $envOptions.find('select').val(environment);
    $envOptions.find('.origin-ux-drop-down-selection span').text($envOptions.find('select > option:selected').text());
	
	$envOptions.find('select').trigger('change');
	var storeEnvironment = window.originDeveloper.storeEnvironment().toLowerCase();
    if (storeEnvironment === 'geoip-prod' || (storeEnvironment === 'review-prod' && environment === 'integration'))
    {
        storeEnvironment = '';
    }
	
    if($storeOptions.find('select.active > option[value="'+storeEnvironment+'"]').length == 0)
    {
        $storeOptions.find('select.active > option:last').before($('<option></option>').attr('value', storeEnvironment).text(storeEnvironment));
        $storeOptions.find('.origin-ux-drop-down-selection span').text(storeEnvironment);
        $storeOptions.find('select.active').val(storeEnvironment);
    }
    else
    {
        $storeOptions.find('select.active').val(storeEnvironment);
        $storeOptions.find('.origin-ux-drop-down-selection span').text($storeOptions.find('select.active > option:selected').text());
    }
	
	var overridePromos = window.originCommon.readOverride('[Connection]', 'OverridePromos');
	var $promo = $('#div-promo');
	
    if($promo.find(' select > option[value="'+overridePromos+'"]').length != 0)
    {
        $promo.find(' select').val(overridePromos);
        $promo.find('.origin-ux-drop-down-selection span').text($promo.find('select > option:selected').text());
    }

	var geoLocation = window.originCommon.readOverride('[Services]', 'GeolocationIPAddress');
	var $geoIp = $('#div-geoip');
    
	$geoIp.find('select').change(function()
    {
		if($(this).val() == 'other')
        {
			$('#txt-geoip-custom-container').show().addClass('important');
		}
		else
        {
			$('#txt-geoip-custom-container').hide().removeClass('important');
		}

	    // https://developer.origin.com/support/browse/ORPUB-1708
        // Hide this UI for now since store has not yet hooked up this functionality and it is confusing game teams
		/*if($(this).val() != '')
        {
            $('#div-geo-test').show();
			$('#txt-geo-test-custom-container').addClass('important');
		}
		else
        {
            $('#div-geo-test').hide();
			$('#txt-geo-test-custom-container').removeClass('important');
		}*/
	});
	
    if($geoIp.find('select > option[value="'+geoLocation+'"]').length === 0)
    {
        $('#txt-geoip-custom-container').show().addClass('important');
        $('#txt-geoip-custom').val(geoLocation);
        geoLocation = 'other';
    }
    
    $geoIp.find('select').val(geoLocation);
    $geoIp.find('.origin-ux-drop-down-selection span').text($geoIp.find('select > option:selected').text());
    
	var geoTest = window.originCommon.readOverride('[Services]', 'OriginGeolocationTest');
    $('#txt-geo-test').val(geoTest);
    if(geoLocation.length !== 0)
    {
        $('#div-geo-test').show();
    }
	
	var cdn = window.originCommon.readOverride('[Feature]', 'CdnOverride');
	var $cdnDiv = $('#div-cdn');
	
    if($cdnDiv.find(' select > option[value="'+cdn+'"]').length !== 0)
    {
        $cdnDiv.find('select').val(cdn);
        $cdnDiv.find('.origin-ux-drop-down-selection span').text($cdnDiv.find('select > option:selected').text());
    }
	
	var motd = window.originCommon.readOverride('[Feature]', 'DisableMotd');
    $('#chk-motd-suppression').prop('checked', motd === 'true');
	
	var flushCache = window.originCommon.readOverride('[Connection]', 'FlushWebCache');
    $('#chk-web-cache-flush').prop('checked', flushCache === 'true');

	var downloadDebug = window.originCommon.readOverride('[QA]', 'DownloadDebugEnabled');
    $('#chk-download-debug').prop('checked', downloadDebug === 'true');
    
	var webDebug = window.originCommon.readOverride('[URLs]', 'WebDebugEnabled');
    $('#chk-web-debug').prop('checked', webDebug === 'true');
	
	var purgeLicenses = window.originCommon.readOverride('[QA]', 'PurgeAccessLicenses');
    $('#chk-purge-licenses').prop('checked', purgeLicenses === 'true');
    
	var disableEmbargoMode = window.originCommon.readOverride('[QA]', 'DisableEmbargoMode');
    $('#chk-rd-mode').prop('checked', disableEmbargoMode === 'true');
    
    var showNetPromoter = window.originCommon.readOverride('[QA]', 'ShowNetPromoter');
    $('#chk-net-promoter').prop('checked', showNetPromoter === 'true');
    
    var enableTelemetryXml = window.originCommon.readOverride('[Telemetry]', 'TelemetryXML');
    $('#chk-telemetry-xml').prop('checked', enableTelemetryXml === 'true');
    
    var showDebugMenu = window.originCommon.readOverride('[QA]', 'ShowDebugMenu');
    $('#chk-debug-menu').prop('checked', showDebugMenu === 'true');

    var forceIGOLogging = window.originCommon.readOverride('[IGO]', 'ForceFullIGOLogging');
    $('#chk-force-igo-logging').prop('checked', forceIGOLogging === 'true');
    
    var enableProgressiveInstall = window.originCommon.readOverride('[ProgressiveInstall]', 'EnableProgressiveInstall');
    $('#chk-disable-prog-install').prop('checked', enableProgressiveInstall === 'false');
    
    var enableSteppedDownload = window.originCommon.readOverride('[ProgressiveInstall]', 'EnableSteppedDownload');
    $('#chk-stepped-download').prop('checked', enableSteppedDownload === 'true');    
};

GeneralOverrideViews.prototype.setGeneralOverrides = function()
{
	var c = Origin.model.constants;
	
	var selectedLocale = '';
	
	// Environment Dropdown
    var environmentName;
	if($('#txt-env-custom-container').hasClass('important'))
    {
		environmentName = $('#txt-env-custom').val().replace(/\s+/g,'');
	}
	else
    {
		environmentName = $('#div-environment').find('select').val();
	}
    window.originCommon.writeOverride('[Connection]', 'EnvironmentName', environmentName);
	
	// Promo Manager
	window.originCommon.writeOverride('[Connection]', 'OverridePromos', $('#div-promo').find('select').val());
	
    // CDN dropdown
	window.originCommon.writeOverride('[Feature]', 'CdnOverride', $('#div-cdn').find('select').val());
	
	// GeoIp Dropdown
    var geoIp = $('#div-geoip').find('select').val();
	if($('#txt-geoip-custom-container').hasClass('important'))
    {
        window.originCommon.writeOverride('[Services]', 'GeolocationIPAddress', $('#txt-geoip-custom').val().replace(/\s+/g,''));
	}
	else
    {
        window.originCommon.writeOverride('[Services]', 'GeolocationIPAddress', geoIp);
	}
    
    if(geoIp === '' || geoIp === 'other')
    {
        selectedLocale = this.customLocale;
    }
    else
    {
        selectedLocale = $('#div-geoip').find('select').find('option:selected').attr('data-locale');
    }

    var geoTest = (geoIp === '') ? '' : $('#txt-geo-test').val();
    window.originCommon.writeOverride('[Services]', 'OriginGeolocationTest', geoTest);

    // 1. If user has specified a store environment, set URLs to match it.
    // 2. If user has specified a geo-IP, use special geo-IP-unlocked URLs to accommodate
    // 3. If user didn't specify either, unset store URLs so that client uses default configured URLs.
	var storeEnvironment = $('#div-store-environment').find('select.active').val();
    if (storeEnvironment.length === 0 && selectedLocale.length > 0)
    {
        if (environmentName.length === 0)
        {
            storeEnvironment = 'geoip-prod';
        }
        else if (environmentName === 'integration')
        {
            storeEnvironment = 'review-prod';
        }
    }

    var storeInitialURL = '';
    var pdlcStoreURL = '';
    var storeProductURL = '';
    var storeMasterTitleURL = '';
    var freeGamesURL = '';
    var entitleFreeGamesURL = '';
    var onTheHouseURL = '';
    var checkoutURL = '';
    
    var storeUrls = window.originDeveloper.storeUrls(storeEnvironment);
    if (Object.keys(storeUrls).length > 0)
    {
        storeInitialURL = storeUrls['storeInitialURL'];
        pdlcStoreURL = storeUrls['pdlcStoreURL'];
        storeProductURL = storeUrls['storeProductURL'];
        storeMasterTitleURL = storeUrls['storeMasterTitleURL'];
        freeGamesURL = storeUrls['freeGamesURL'];
        entitleFreeGamesURL = storeUrls['entitleFreeGamesURL'];
        onTheHouseURL = storeUrls['onTheHouseURL'];
        checkoutURL = storeUrls['checkoutURL'];
    }
    
    // Store URLs have been set by this point, but still contain a {locale} token.
    // Replace this will the appropriate locale (or remove it if none specified).
    if(selectedLocale.length > 0)
    {
        storeInitialURL = storeInitialURL.replace('{locale}', selectedLocale);
        pdlcStoreURL = pdlcStoreURL.replace('{locale}', selectedLocale);
        storeProductURL = storeProductURL.replace('{locale}', selectedLocale);
        storeMasterTitleURL = storeMasterTitleURL.replace('{locale}', selectedLocale);
        freeGamesURL = freeGamesURL.replace('{locale}', selectedLocale);
        entitleFreeGamesURL = entitleFreeGamesURL.replace('{locale}', selectedLocale);
        onTheHouseURL = onTheHouseURL.replace('{locale}', selectedLocale);
        checkoutURL = checkoutURL.replace('{locale}', selectedLocale);
    }
    else
    {
        storeInitialURL = storeInitialURL.replace('{locale}', '');
        pdlcStoreURL = pdlcStoreURL.replace('{locale}', '');
        storeProductURL = storeProductURL.replace('{locale}', '');
        storeMasterTitleURL = storeMasterTitleURL.replace('{locale}', '');
        freeGamesURL = freeGamesURL.replace('{locale}', '');
        entitleFreeGamesURL = entitleFreeGamesURL.replace('{locale}', '');
        onTheHouseURL = onTheHouseURL.replace('{locale}', '');
        checkoutURL = checkoutURL.replace('{locale}', '');
    }
    
    window.originCommon.writeOverride('[URLs]', 'StoreInitialURL', storeInitialURL);
    window.originCommon.writeOverride('[URLs]', 'PdlcStoreURL', pdlcStoreURL);
    window.originCommon.writeOverride('[URLs]', 'StoreProductURL', storeProductURL);
    window.originCommon.writeOverride('[URLs]', 'StoreMasterTitleURL', storeMasterTitleURL);
    window.originCommon.writeOverride('[URLs]', 'FreeGamesURL', freeGamesURL);
    window.originCommon.writeOverride('[URLs]', 'StoreEntitleFreeGameURL', entitleFreeGamesURL);
    window.originCommon.writeOverride('[URLs]', 'OnTheHouseStoreURL', onTheHouseURL);
    window.originCommon.writeOverride('[URLs]', 'EbisuLockboxV3URL', checkoutURL);
	
	var $motd = $('#chk-motd-suppression');
    window.originCommon.writeOverride('[Feature]', 'DisableMotd', $motd.attr('checked') ? 'true' : '');
	
	var $flush = $('#chk-web-cache-flush');
    window.originCommon.writeOverride('[Connection]', 'FlushWebCache', $flush.attr('checked') ? 'true' : '');

	var $downloadDebug = $('#chk-download-debug');
    window.originCommon.writeOverride('[QA]', 'DownloadDebugEnabled', $downloadDebug.attr('checked') ? 'true' : '');
    
	var $webDebug = $('#chk-web-debug');
    window.originCommon.writeOverride('[URLs]', 'WebDebugEnabled', $webDebug.attr('checked') ? 'true' : '');
	
	var $purgeLicenses = $('#chk-purge-licenses');
    window.originCommon.writeOverride('[QA]', 'PurgeAccessLicenses', $purgeLicenses.attr('checked') ? 'true' : '');
    
	var $rdMode = $('#chk-rd-mode');
    window.originCommon.writeOverride('[QA]', 'DisableEmbargoMode', $rdMode.attr('checked') ? 'true' : '');
    
	var $netPromoter = $('#chk-net-promoter');
    window.originCommon.writeOverride('[QA]', 'ShowNetPromoter', $netPromoter.attr('checked') ? 'true' : '');
    
	var $telemetryXml = $('#chk-telemetry-xml');
    window.originCommon.writeOverride('[Telemetry]', 'TelemetryXML', $telemetryXml.attr('checked') ? 'true' : '');
    
	var $debugMenu = $('#chk-debug-menu');
    window.originCommon.writeOverride('[QA]', 'ShowDebugMenu', $debugMenu.attr('checked') ? 'true' : '');

    var $forceIGOLogging = $('#chk-force-igo-logging');
    window.originCommon.writeOverride('[IGO]', 'ForceFullIGOLogging', $forceIGOLogging.attr('checked') ? 'true' : '');
    
	var $disableProgInstall = $('#chk-disable-prog-install');
    window.originCommon.writeOverride('[ProgressiveInstall]', 'EnableProgressiveInstall', $disableProgInstall.attr('checked') ? 'false' : '');    
    
	var $steppedDownload = $('#chk-stepped-download');
    window.originCommon.writeOverride('[ProgressiveInstall]', 'EnableSteppedDownload', $steppedDownload.attr('checked') ? 'true' : '');
};

Origin.views.generalOverrides = new GeneralOverrideViews();

})(jQuery);