;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

var getSelectedSoftwareBuild = function(softwareBuildTable)
{
	var selectedBuildTR = softwareBuildTable.find('.build-override-checkbox:checked');
	if(selectedBuildTR.length) 
    {
		console.log("selected build id was: " + selectedBuildTR.attr('build-id'));
		return selectedBuildTR.attr('build-id');
	}
	else 
    {
		console.log("no build id is selected.");
		return '';
	}
};

var GameDetailsOverrideViews = function()
{    
	$('#game-details-footer').find('#btn-save').on('click', function(event)
    {
		Origin.views.gameDetailsOverride.setGameDetails();
        Origin.views.myGamesOverrides.init();
        Origin.views.navbar.currentTab = "my-games";
	});
	
	$('#game-details-footer').find('#btn-cancel').on('click', function(event)
    {
        Origin.views.navbar.currentTab = "my-games";
	});
	
	$('#btn-reset-trial-license').on('click', function(event)
    {
		if($(this).prop('disabled') == false)
		{
            window.originDeveloper.launchTrialResetTool($(this).attr('content-id'));
        }
	});
};

GameDetailsOverrideViews.prototype.init = function(id)
{
    Origin.views.gameDetailsOverride.clearGameDetails();
    
    console.info(id);
    var entitlement = window.entitlementManager.getEntitlementById(id);
	if(entitlement !== null)
    {
		console.info(entitlement);
		Origin.views.gameDetailsOverride.renderGameDetails(entitlement);
	}
    else
    {
        var unownedQuery = productQuery.startQuery(id);
        unownedQuery.succeeded.connect(function(product)
        {
            console.info('Product query succeeded for ' + product.productId);
            Origin.views.gameDetailsOverride.renderGameDetails(product);
        });
        
        unownedQuery.failed.connect(function(id)
        {
            console.info('Product query failed for ' + id);
            var product = $.extend({}, Origin.model.constants.DUMMY_ENTITLEMENT);
            product.productId = id;
            Origin.views.gameDetailsOverride.renderGameDetails(product);
        });
    }
	Origin.util.badImageRender();
};

GameDetailsOverrideViews.prototype.clearGameDetails = function()
{
	$('#cloud-info').hide();
    $('#software-builds').css('display', 'none');
    
	$('#dlc-overrides').html('');
	$('#content-id').html('');
	$('#offer-id').html('');
	$('#item-id').html('');
	$('#master-title-id').html('');
    
	$('#game-title-header').html('');
    $('#game-details').find('#game-boxart > img').attr('src', Origin.model.constants.DEFAULT_IMAGE_PATH);
	$('#txt-local-override').val('');
	$('#txt-local-override').removeAttr('disabled');
	$('#btn-local-override').prop('disabled', false);
	$('#txt-sync-package').val('');
	$('#txt-sync-package').removeAttr('disabled');
	$('#btn-sync-package').prop('disabled', false);
	$('#txt-local-version').val('');
	$('#txt-local-version').removeAttr('disabled');
	$('#txt-shared-network').val('');
	$('#txt-shared-network').removeAttr('disabled');
	$('#btn-shared-network').prop('disabled', false);
	$('#txt-scheduled-time').val('');
	$('#txt-scheduled-time').removeAttr('disabled');    
    $('#chk-confirm-build').prop('disabled', false);
    $('#chk-confirm-build').prop('checked', false);    
	$('#txt-installer-name').val('');
	$('#txt-installer-name').removeAttr('disabled');
	$('#txt-installation-location').val('');
	$('#txt-installation-location').removeAttr('disabled');
	$('#txt-game-executable-name').val('');
	$('#txt-game-executable-name').removeAttr('disabled');
    $('#cloud-configuration-textarea').html('');
	$('#btn-clear-remote-cloud').prop('disabled', false);
    $('#chk-twitch-blacklist').prop('disabled', false);
    $('#chk-twitch-blacklist').prop('checked', false);
    $('#txt-achievement-set-override').val('');
	$('#txt-achievement-set-override').removeAttr('disabled');
    $('#btn-achievement-set-override').prop('disabled', false);
    
	var $updateCheck = $('#div-update-check');
    $updateCheck.find(' select').val('latest');
    $updateCheck.find('.origin-ux-drop-down-selection span').text($updateCheck.find('select > option:selected').text());
}

GameDetailsOverrideViews.prototype.updateOverrideInputState = function(offerId)
{
    var sharedNetworkFolder = $('#txt-shared-network[data-id="' + offerId + '"]').val();
    var buildId = $('#software-builds[data-id="' + offerId + '"]').find('.build-override-checkbox:checked').attr('build-id');
    var downloadPath = $('#txt-local-override[data-id="' + offerId + '"]').val();

    var sharedNetworkInputEnabled = true;
    var localOverrideInputEnabled = true;
    var buildOverrideInputEnabled = true;

    if(sharedNetworkFolder !== '' && sharedNetworkFolder !== undefined)
    {
        localOverrideInputEnabled = false;
        buildOverrideInputEnabled = false;
    }
    else if(buildId !== '' && buildId !== undefined)
    {
        sharedNetworkInputEnabled = false;
        localOverrideInputEnabled = false;
    }
    else if(downloadPath !== '' && downloadPath !== undefined)
    {
        sharedNetworkInputEnabled = false;
        buildOverrideInputEnabled = false;
    }

    // Enable/disable checkboxes in software build table.
    $('#software-builds[data-id="' + offerId + '"]').find('.build-override-checkbox').each(function()
    {
        if(buildOverrideInputEnabled)
        {
            $(this).prop('disabled', $(this).attr('default-state') === 'disabled');
        }
        else
        {
            $(this).prop('disabled', true);
        }
    });

	// Disable local override and local override version for this offer
    if(localOverrideInputEnabled) 
    {
        $('#btn-local-override[data-id="' + offerId + '"]').prop('disabled', false);
        $('#btn-sync-package[data-id="' + offerId + '"]').prop('disabled', false);
        $('#txt-local-override[data-id="' + offerId + '"]').removeAttr('disabled');
        $('#txt-sync-package[data-id="' + offerId + '"]').removeAttr('disabled');
        $('#txt-local-version[data-id="' + offerId + '"]').removeAttr('disabled');
        $('#txt-local-override[data-id="' + offerId + '"]').parent().parent().parent().removeClass('disabled');
        $('#txt-sync-package[data-id="' + offerId + '"]').parent().parent().parent().removeClass('disabled');
        $('#txt-local-version[data-id="' + offerId + '"]').parent().parent().parent().removeClass('disabled');
    }
    else 
    {
        $('#btn-local-override[data-id="' + offerId + '"]').prop('disabled', true);
        $('#btn-sync-package[data-id="' + offerId + '"]').prop('disabled', true);
        $('#txt-local-override[data-id="' + offerId + '"]').attr('disabled', 'disabled');
        $('#txt-sync-package[data-id="' + offerId + '"]').attr('disabled', 'disabled');
        $('#txt-local-version[data-id="' + offerId + '"]').attr('disabled', 'disabled');
        $('#txt-local-override[data-id="' + offerId + '"]').parent().parent().parent().addClass('disabled');
        $('#txt-sync-package[data-id="' + offerId + '"]').parent().parent().parent().addClass('disabled');
        $('#txt-local-version[data-id="' + offerId + '"]').parent().parent().parent().addClass('disabled');
    }

    // Disable shared network overrides for this offer
    if(sharedNetworkInputEnabled) 
    {
        $('#btn-shared-network[data-id="' + offerId + '"]').prop('disabled', false);
        $('#txt-shared-network[data-id="' + offerId + '"]').removeAttr('disabled');
        $('#txt-shared-network[data-id="' + offerId + '"]').parent().parent().parent().removeClass('disabled');

        // Enable or disable dropdown-update-check and chk-confirm-build based on txt-shared-network's value
        if(sharedNetworkFolder !== '' && sharedNetworkFolder !== undefined)
        {
            $('#dropdown-update-check[data-id="' + offerId + '"]').removeClass('disabled');
            $('#chk-confirm-build[data-id="' + offerId + '"]').prop("disabled", false);

            // This will enable or disable txt-scheduled-time based on dropdown-update-check's value
            $('#dropdown-update-check[data-id="' + offerId + '"] select').trigger('change');
        }
        else
        {
            $('#dropdown-update-check[data-id="' + offerId + '"]').addClass('disabled');
            $('#chk-confirm-build[data-id="' + offerId + '"]').prop("disabled", true);
            $('#chk-confirm-build[data-id="' + offerId + '"]').prop('checked', false);

            $('#txt-scheduled-time[data-id="' + offerId + '"]').attr('disabled', 'disabled');
            $('#txt-scheduled-time[data-id="' + offerId + '"]').parent().parent().parent().addClass('disabled');
            $('#txt-scheduled-time[data-id="' + offerId + '"]').val('');
        }
    }
    else 
    {
        $('#btn-shared-network[data-id="' + offerId + '"]').prop('disabled', true);
        $('#txt-shared-network[data-id="' + offerId + '"]').attr('disabled', 'disabled');
        $('#txt-shared-network[data-id="' + offerId + '"]').parent().parent().parent().addClass('disabled');

        $('#dropdown-update-check[data-id="' + offerId + '"]').addClass('disabled');
        $('#chk-confirm-build[data-id="' + offerId + '"]').prop("disabled", true);

        $('#txt-scheduled-time[data-id="' + offerId + '"]').attr('disabled', 'disabled');
        $('#txt-scheduled-time[data-id="' + offerId + '"]').parent().parent().parent().addClass('disabled');
        $('#txt-scheduled-time[data-id="' + offerId + '"]').val('');
    }
}

GameDetailsOverrideViews.prototype.renderGameDetails = function(entitlement)
{    
    console.info("Showing game details for: " + entitlement.productId);
    console.info(entitlement);
    
    // Populate data-id attribute for any element that needs it
	$('#base-game').find('[data-id]').each(function() 
    { 
		$(this).attr('data-id', entitlement.productId);
	});

    $('#game-title-header').html(entitlement.title);
    $('#content-id').html(entitlement.contentId);
    $('#offer-id').html(entitlement.productId);
    $('#item-id').html(entitlement.expansionId);
    $('#master-title-id').html(entitlement.masterTitleId);
    
    $('#game-boxart > img').attr('src', entitlement.boxartUrls.length > 0 ? entitlement.boxartUrls[0] : Origin.model.constants.DEFAULT_IMAGE_PATH);
    $('#game-boxart > img').attr('title', entitlement.title);
    
    this.renderSoftwareBuilds(entitlement);
    
    var downloadPath = window.originCommon.readOverride('[Connection]', 'OverrideDownloadPath', entitlement.productId);
    var sharedNetworkFolder = window.originCommon.readOverride('[Connection]', 'OverrideSNOFolder', entitlement.productId);
    if(sharedNetworkFolder !== '')
    {
        Origin.util.readOverride('[Connection]', 'OverrideSNOFolder', entitlement.productId, $('#txt-shared-network'));
        Origin.util.readOverride('[Connection]', 'OverrideSNOScheduledTime', entitlement.productId, $('#txt-scheduled-time'));
        Origin.util.readOverride('[Connection]', 'OverrideSNOConfirmBuild', entitlement.productId, $('#chk-confirm-build'));
        Origin.util.readOverride('[Connection]', 'OverrideSNOUpdateCheck', entitlement.productId, $('#dropdown-update-check'));
    }
    else if(downloadPath !== '')
    {
        Origin.util.readOverride('[Connection]', 'OverrideDownloadPath', entitlement.productId, $('#txt-local-override'));
        Origin.util.readOverride('[Connection]', 'OverrideDownloadSyncPackagePath', entitlement.productId, $('#txt-sync-package'));
        Origin.util.readOverride('[Connection]', 'OverrideVersion', entitlement.productId, $('#txt-local-version'));
    }

    Origin.util.readOverride('[Connection]', 'OverrideInstallFilename', entitlement.productId, $('#txt-installer-name'));
    Origin.util.readOverride('[Connection]', 'OverrideExecuteFilename', entitlement.productId, $('#txt-game-executable-name'));
    Origin.util.readOverride('[Connection]', 'OverrideGameLauncherURL', entitlement.productId, $('#txt-game-launcher-url'));
    Origin.util.readOverride('[Connection]', 'OverrideGameLauncherURLIntegration', entitlement.productId, $('#txt-game-launcher-url-int'));
    Origin.util.readOverride('[Connection]', 'OverrideGameLauncherURLIdentityClientId', entitlement.productId, $('#txt-game-launcher-client-id'));
    Origin.util.readOverride('[Connection]', 'OverrideGameLauncherSoftwareId', entitlement.productId, $('#txt-game-launcher-software-id'));
    Origin.util.readOverride('[Connection]', 'OverrideAchievementSet', entitlement.productId, $('#txt-achievement-set-override'));
    
    // These two should be kept in sync.
    var installRegistryKey = window.originCommon.readOverride('[Connection]', 'OverrideInstallRegistryKey', entitlement.productId);
    if(installRegistryKey === '')
    {
        installRegistryKey = window.originCommon.readOverride('[Connection]', 'OverrideExecuteRegistryKey', entitlement.productId);
    }
    $('#txt-installation-location').val(installRegistryKey);

    var cloudConfiguration = window.originDeveloper.cloudConfiguration(entitlement.productId);
    if(cloudConfiguration.length > 0)
    {
        $('#cloud-info').show();
        $('#cloud-configuration-textarea').html(cloudConfiguration.join('<br>'));
        
        // Clear previously registered click handlers in the DOM, before setting a new one.
        $('#btn-clear-remote-cloud').off('click.clearCloud').on('click.clearCloud', function(event) {
            window.originDeveloper.clearRemoteArea(entitlement.productId);
        });
    }
    else
    {
        $('#cloud-info').hide();
        $('#cloud-configuration-textarea').html('');
    }
    
    // Game is blacklisted if redeemed with test code (1102/1103). OR unreleased OR blacklisted (via server config) OR a NOG (in R&D mode)
    if (entitlement.testCode !== null || entitlement.releaseStatus !== 'AVAILABLE' || entitlement.twitchBlacklisted || entitlement.itemSubType === 'NON_ORIGIN')
    {
        $('#chk-twitch-blacklist').prop('disabled', true);
    }
    else
    {
        $('#chk-twitch-blacklist').prop('disabled', false);
    }
    
    Origin.util.readOverride('[QA]', 'DisableTwitchBlacklist', $('#chk-twitch-blacklist'));
    
    // Only disable the button if the user is actually on production.  They may have previously changed
    // the environment to production via dropdown but declined to restart.
    var isProduction = (window.clientSettings.readSetting('EnvironmentName').toLowerCase() === 'production');
    $('#btn-reset-trial-license').prop('disabled', isProduction);
    $('#btn-reset-trial-license').attr('content-id', entitlement.contentId);
                   
    if(entitlement.itemSubType !== 'LIMITED_WEEKEND_TRIAL' && 
       entitlement.itemSubType !== 'FREE_WEEKEND_TRIAL' && 
       entitlement.itemSubType !== 'LIMITED_TRIAL')
    {
        $('#div-reset-trial-license').hide();
    }
    
    if(entitlement.expansions !== undefined)
    {
        $.each(entitlement.expansions, $.proxy(function(id, expansion)
        {
            this.renderEachAddon(expansion);
        }, this));
    }
    
    if(entitlement.addons !== undefined)
    {
        $.each(entitlement.addons, $.proxy(function(id, addon)
        {
            this.renderEachAddon(addon);
        }, this));
    }
    
    $('#game-information').find('#txt-local-override').each(function()
    {
        function handleEvent()
        {
            if($(this).prop('disabled') == false) 
            {
                Origin.views.gameDetailsOverride.updateOverrideInputState($(this).attr('data-id'));
            }
        };

        $(this).off('keyup').on('keyup', handleEvent);
        $(this).off('change').on('change', handleEvent); 
    });
    
	$('#game-information').find('#btn-local-override').each(function()
    {
        $(this).off('click').on('click', function(event)
        {
            if($(this).prop('disabled') == false) 
            {
                var selectedFile = window.originDeveloper.selectFile('desktop', '*.zip *.dmg' , 'open', 'local-override');
                if(selectedFile != '')
                {
                    var $input = $(this).parent().find('span > span > input');
                    $input.val(selectedFile);
                    $input.trigger('change');
                }
            }
        });
    });
    
	$('#game-information').find('#btn-sync-package').each(function()
    {
        $(this).off('click').on('click', function(event)
        {
            if($(this).prop('disabled') == false) 
            {
                var selectedFile = window.originDeveloper.selectFile('desktop', '*.zip *.dmg' , 'open', 'sync-package-override');
                if(selectedFile != '')
                {
                    var $input = $(this).parent().find('span > span > input');
                    $input.val(selectedFile);
                    $input.trigger('change');
                }
            }
        });
    });
    
    $('#game-information').find('#txt-shared-network').each(function()
    {
        function handleEvent()
        {
            if($(this).prop('disabled') == false) 
            {
                Origin.views.gameDetailsOverride.updateOverrideInputState($(this).attr('data-id'));
            }
        };

        $(this).off('keyup').on('keyup', handleEvent);
        $(this).off('change').on('change', handleEvent);
    });
    
	$('#game-information').find('#btn-shared-network').each(function()
    {
        $(this).off('click').on('click', function(event)
        {
            if($(this).prop('disabled') == false) 
            {
                var selectedFolder = window.originDeveloper.selectFolder();
                if(selectedFolder != '')
                {
                    var $input = $(this).parent().find('span > span > input');
                    $input.val(selectedFolder);
                    $input.trigger('change');
                }
            }
        });
    });    

    $('#game-information').find('#dropdown-update-check select').each(function()
    {
        $(this).off('change').on('change', function(event)
        {
            var offerId = $(this).parent().parent().attr('data-id');

            if($(this).val() == "scheduled")
            {
                $('#txt-scheduled-time[data-id="' + offerId + '"]').removeAttr('disabled', 'disabled');
                $('#txt-scheduled-time[data-id="' + offerId + '"]').parent().parent().parent().removeClass('disabled');
            }
            else
            {
                $('#txt-scheduled-time[data-id="' + offerId + '"]').attr('disabled', 'disabled');
                $('#txt-scheduled-time[data-id="' + offerId + '"]').parent().parent().parent().addClass('disabled');
                $('#txt-scheduled-time[data-id="' + offerId + '"]').val('');
            }
        });
    });
    
    // Determine the platformId - used in generating the default achievement set ID
    var platformId;
    var platform = Origin.views.currentPlatform();
    
    if (platform === 'PC')
    {
        platformId = 50844;
    }
    else if (platform === 'MAC')
    {
        platformId = 50593;
    }
    
    // Clear previously registered click handlers in the DOM, before setting a new one.
    $('#btn-achievement-set-override').off('click').on('click', function(event) {
        $('#txt-achievement-set-override').val(entitlement.franchiseId + '_' + entitlement.masterTitleId + '_' + platformId);
    });

    // Update base game fields' state based on populated values.
    Origin.views.gameDetailsOverride.updateOverrideInputState(entitlement.productId);
};

GameDetailsOverrideViews.prototype.renderSoftwareBuilds = function(entitlement)
{
    var softwareBuildMap = window.originDeveloper.softwareBuildMap(entitlement.productId);
    var buildId = window.originCommon.readOverride('[Connection]', 'OverrideBuildIdentifier', entitlement.productId);
    var buildReleaseVersion = window.originCommon.readOverride('[Connection]', 'OverrideBuildReleaseVersion', entitlement.productId);
    
    if(entitlement.owned && !$.isEmptyObject(softwareBuildMap))
    {
        $('#software-builds[data-id="' + entitlement.productId + '"]').show();
        
        // No checkbox if there is only one option and it's the live build.
        var noCheckboxes = (Object.keys(softwareBuildMap).length == 1) && (softwareBuildMap[0].urlType === 'Live');
        
        // Sort by live date since version number is a string ('1' < '10' < '2')
        softwareBuildMap.sort($.proxy(function(a, b)
        {
            var prop = Origin.model.constants.DEFAULT_SOFTWARE_BUILD_SORT_BY;
            var direction = Origin.model.constants.DEFAULT_SOFTWARE_BUILD_SORTING_DIRECTION;
            
            if (a[prop] > b[prop])
            {
                return direction;
            }
            else
            {
                return direction * -1;
            }
        }, this));
        
        var html = '';
        var boxChecked = false;
        $.each(softwareBuildMap, $.proxy(function(index, buildInfo)
        {
            html += "<tr class='software-build-row-data'>";
            html += "<td class='selectable'> " + buildInfo['buildLiveDate'] + " </td>";
            html += "<td class='selectable'> " + buildInfo['urlType'] + " </td>";
            html += "<td class='selectable'> " + buildInfo['buildReleaseVersion'] + " </td>";
            html += "<td class='selectable'> " + buildInfo['fileName'] + " </td>";
            html += "<td class='selectable' " + (noCheckboxes ? "colspan='2'" : "") + ">  " + (buildInfo['buildNotes'].length > 0 ? buildInfo['buildNotes'] : "&nbsp;") + " </td>";
            
            var checkedProp = '';		
            var buildIdMatches = buildId !== '' && buildId === buildInfo['buildId'];
            var buildReleaseVersionMatches = buildReleaseVersion !== '' && buildReleaseVersion === buildInfo['buildReleaseVersion'];
            if(!boxChecked && (buildIdMatches || buildReleaseVersionMatches))
            {
                checkedProp = 'checked="checked"';
                boxChecked = true;
            }
            
            var disabledProp = '';
            var defaultState = 'enabled';
            var futureBuild = buildInfo['buildLiveDate'] > Date.now();
            var hasPermissions = entitlement.testCode === '1103' || (entitlement.testCode === '1102' && entitlement.releaseStatus === 'AVAILABLE');
            if(futureBuild && !hasPermissions)
            {
                disabledProp = 'disabled';
                defaultState = 'disabled';
            }
            
            if(!noCheckboxes) 
            {
                var checkboxHtml = '<input type="checkbox" data-id="' + entitlement.productId + '" build-id="' + buildInfo['buildId'] + '" default-state="' + defaultState + '" class="build-override-checkbox" ' + checkedProp + disabledProp + '/>';
                html += "<td width='10px' text-align='center'>" + checkboxHtml + "</td>";
            }
            
            html += "</tr>";
        }, this));
        
        $('#software-build-data[data-id="' + entitlement.productId + '"]').find('tbody').html(html);
        Origin.views.gameDetailsOverride.updateOverrideInputState(entitlement.productId);

        $('#software-build-data[data-id="' + entitlement.productId + '"]').find('.build-override-checkbox').each(function()
        {
            $(this).off('change').on('change', function(event) 
            {
                var offerId = $(this).attr('data-id');
                var wasChecked = $(this).prop('checked');
                
                var $softwareBuildTable = $('#software-builds[data-id="' + offerId + '"]');
                $softwareBuildTable.find('.build-override-checkbox').removeAttr('checked');
                $(this).prop('checked', wasChecked);
            
                Origin.views.gameDetailsOverride.updateOverrideInputState(offerId);
            });
        });
        
        var dip = entitlement.packageType === 'DOWNLOAD_IN_PLACE' || entitlement.packageType === 'ORIGIN_PLUGIN';
        $('#btn-download-crc[data-id="' + entitlement.productId + '"]').prop('disabled', !dip);
        $('#btn-view-installer-xml[data-id="' + entitlement.productId + '"]').prop('disabled', !dip);

        $('#btn-view-installer-xml[data-id="' + entitlement.productId + '"]').off('click').on('click', function(event)
        {
            // TO-DO - update for shared network builds?
            var offerId = $(this).attr("data-id");
            var localOverride = Origin.util.addFilePrefix($('#txt-local-override[data-id="' + offerId + '"]').val());
            var selectedBuildId = getSelectedSoftwareBuild($('#software-build-data[data-id="' + offerId + '"]'));
            window.originDeveloper.downloadBuildInstallerXml(offerId, selectedBuildId, localOverride);
        });

        $('#btn-download-from-browser[data-id="' + entitlement.productId + '"]').off('click').on('click', function(event)
        {
            var offerId = $(this).attr("data-id");
            var selectedBuildId = getSelectedSoftwareBuild($('#software-build-data[data-id="' + offerId + '"]'));
            window.originDeveloper.downloadBuildInBrowser(offerId, selectedBuildId);
        });
        
        $('#btn-download-crc[data-id="' + entitlement.productId + '"]').off('click').on('click', function(event)
        {
            var offerId = $(this).attr("data-id");
            var selectedBuildId = getSelectedSoftwareBuild($('#software-build-data[data-id="' + offerId + '"]'));
            window.originDeveloper.downloadCrc(offerId, selectedBuildId);
        });
    }
    else
    {
        $('#software-builds[data-id="' + entitlement.productId + '"]').hide();
        Origin.views.gameDetailsOverride.updateOverrideInputState(entitlement.productId);
    }
};

GameDetailsOverrideViews.prototype.renderEachAddon = function(addon)
{	
	var dlcTemplate = $('#dlc-template').clone();
	
	dlcTemplate.attr('id', 'dlc-item');
	dlcTemplate.attr('data-id', addon.productId);
	dlcTemplate.find('> .dlc').attr('data-id', addon.productId);
	dlcTemplate.find('> .dlc > header > h2').html(addon.title);
	dlcTemplate.find('> .dlc > header > ul > li > .value').html(addon.productId);
	
    // Populate data-id attribute for any element that needs it
	dlcTemplate.find('[data-id]').each(function() 
    { 
		$(this).attr('data-id', addon.productId);
	});
	
	$('#dlc-overrides').append(dlcTemplate);
    
    Origin.util.readOverride('[Connection]', 'OverrideDownloadPath', addon.productId, dlcTemplate.find('#txt-local-override'));
    Origin.util.readOverride('[Connection]', 'OverrideDownloadSyncPackagePath', addon.productId, dlcTemplate.find('#txt-sync-package'));
    Origin.util.readOverride('[Connection]', 'OverrideInstallFilename', addon.productId, dlcTemplate.find('#txt-local-version'));
    Origin.util.readOverride('[Connection]', 'OverrideVersion', addon.productId, dlcTemplate.find('#txt-install-check'));
	
	// If this is unowned content, display text stating this.
	if(addon.owned)
	{
		dlcTemplate.find('#lbl-unowned-content').hide();
	}
	else
	{
		dlcTemplate.find('#lbl-unowned-content').show();
	}
	
	// If this is PULC, suppress software build selection and download override, display this is PULC note.
	if(addon.isPULC)
	{
		dlcTemplate.find('#software-builds').hide();
        
		dlcTemplate.find('#lbl-pulc-info').show();
		dlcTemplate.find('#download-overrides').hide();
		dlcTemplate.find('#install-overrides').hide();
	}
	else
	{
        // renderSoftwareBuilds will take care of showing/hiding when appropriate.
        this.renderSoftwareBuilds(addon);
        
		dlcTemplate.find('#lbl-pulc-info').hide();
		dlcTemplate.find('#download-overrides').show();
		dlcTemplate.find('#install-overrides').show();
	}
	
	dlcTemplate.css('display', 'block');
    Origin.views.gameDetailsOverride.updateOverrideInputState(addon.productId);
};

GameDetailsOverrideViews.prototype.setGameDetails = function()
{	
    // Save base game overrides
	var offerId = $('#offer-id').html();

	Origin.util.writeOverride('[Connection]', 'OverrideDownloadPath', offerId, $('#txt-local-override[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideDownloadSyncPackagePath', offerId, $('#txt-sync-package[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideVersion', offerId, $('#txt-local-version[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideSNOFolder', offerId, $('#txt-shared-network[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideSNOUpdateCheck', offerId, $('#dropdown-update-check[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideSNOScheduledTime', offerId, $('#txt-scheduled-time[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideSNOConfirmBuild', offerId, $('#chk-confirm-build[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideBuildIdentifier', offerId, $('#software-builds[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideInstallFilename', offerId, $('#txt-installer-name[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideExecuteRegistryKey', offerId, $('#txt-installation-location[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideInstallRegistryKey', offerId, $('#txt-installation-location[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideExecuteFilename', offerId, $('#txt-game-executable-name[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideGameLauncherURL', offerId, $('#txt-game-launcher-url[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideGameLauncherURLIntegration', offerId, $('#txt-game-launcher-url-int[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[Connection]', 'OverrideGameLauncherURLIdentityClientId', offerId, $('#txt-game-launcher-client-id[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideGameLauncherSoftwareId', offerId, $('#txt-game-launcher-software-id[data-id="' + offerId + '"]'));
	Origin.util.writeOverride('[Connection]', 'OverrideAchievementSet', offerId, $('#txt-achievement-set-override[data-id="' + offerId + '"]'));
    Origin.util.writeOverride('[QA]', 'DisableTwitchBlacklist', offerId, $('#chk-twitch-blacklist'));

    // Clear this override since it has been replaced by OverrideBuildIdentifier
    window.originCommon.writeOverride('[Connection]', 'OverrideBuildReleaseVersion', offerId, '');
    
    // Save addon/expansion overrides
	var $container = $('#dlc-overrides');
	var dlc = $container.find('.dlc');
	var count = $container.find('.dlc').length;
	
	if(count > 0)
    {
		dlc.each(function()
        {
			var offerId = $(this).attr('data-id');

            Origin.util.writeOverride('[Connection]', 'OverrideDownloadPath', offerId, $('#txt-local-override[data-id="' + offerId + '"]'));
            Origin.util.writeOverride('[Connection]', 'OverrideDownloadSyncPackagePath', offerId, $('#txt-sync-package[data-id="' + offerId + '"]'));
            Origin.util.writeOverride('[Connection]', 'OverrideVersion', offerId, $('#txt-local-version[data-id="' + offerId + '"]'));
            Origin.util.writeOverride('[Connection]', 'OverrideBuildIdentifier', offerId, $('#software-builds[data-id="' + offerId + '"]'));
            Origin.util.writeOverride('[Connection]', 'OverrideInstallFilename', offerId, $('#txt-install-check[data-id="' + offerId + '"]'));
            
            // Clear this override since it has been replaced by OverrideBuildIdentifier
            window.originCommon.writeOverride('[Connection]', 'OverrideBuildReleaseVersion', offerId, '');
		});
	}
};

Origin.views.gameDetailsOverride = new GameDetailsOverrideViews();

})(jQuery);
