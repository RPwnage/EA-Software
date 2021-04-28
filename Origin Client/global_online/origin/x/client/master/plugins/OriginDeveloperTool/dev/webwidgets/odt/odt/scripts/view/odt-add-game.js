;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

var AddGameOverrideView = function()
{    
	$('#add-game-footer').find('#btn-save').on('click', function(event)
    {
		if(Origin.views.addGameOverride.setGameDetails())
        {
            Origin.views.myGamesOverrides.init();
            Origin.views.navbar.currentTab = "my-games";
        }
	});
	
	$('#add-game-footer').find('#btn-cancel').on('click', function(event)
    {
        Origin.views.navbar.currentTab = "my-games";
	});
	
	// Event delegation for download and install overrides inside main game detail container
	$('#btn-add-game-local-override').on('click', function(event)
    {
		if(!$(this).prop('disabled')) 
        {
			var selectedFile = window.originDeveloper.selectFile('desktop', '*.zip *.dmg' , 'open', 'local-override');
			if(selectedFile.length > 0) 
            {
				$('#txt-add-game-local-override').val(selectedFile);
			}
		}
	});
    
	$('#btn-add-game-sync-package').on('click', function(event)
    {
		if(!$(this).prop('disabled')) 
        {
			var selectedFile = window.originDeveloper.selectFile('desktop', '*.zip *.dmg' , 'open', 'sync-package-override');
			if(selectedFile != '') 
            {
				$('#txt-add-game-sync-package').val(selectedFile);
			}
		}
	});
};

AddGameOverrideView.prototype.init = function()
{
    Origin.views.addGameOverride.clearGameDetails();
}

AddGameOverrideView.prototype.clearGameDetails = function()
{
    $('#add-game').find('#game-boxart > img').attr('src', Origin.model.constants.DEFAULT_IMAGE_PATH);
    
	$('#txt-add-game-unowned-offer-id').val('');
	$('#txt-add-game-unowned-offer-id').removeAttr('disabled');
	$('#txt-add-game-local-override').val('');
	$('#txt-add-game-local-override').removeAttr('disabled');
	$('#btn-add-game-local-override').prop('disabled', false);
	$('#txt-add-game-sync-package').val('');
	$('#txt-add-game-sync-package').removeAttr('disabled');
	$('#btn-add-game-sync-package').prop('disabled', false);
	$('#txt-add-game-local-version').val('');
	$('#txt-add-game-local-version').removeAttr('disabled');
	$('#txt-add-game-installer-name').val('');
	$('#txt-add-game-installer-name').removeAttr('disabled');
	$('#txt-add-game-installation-location').val('');
	$('#txt-add-game-installation-location').removeAttr('disabled');
	$('#txt-add-game-game-executable-name').val('');
	$('#txt-add-game-game-executable-name').removeAttr('disabled');
    
    $('#txt-unowned-offer-id-container').find('> .origin-ux-textbox').removeClass('field-error');
}

AddGameOverrideView.prototype.setGameDetails = function()
{
    var offerId = $('#txt-unowned-offer-id').val();
    console.info('Adding offer: ' + offerId);
    if(offerId == '')
    {
        $('#txt-unowned-offer-id-container').addClass('field-error');
        return false;
    }
    else
    {
        $('#txt-unowned-offer-id-container').removeClass('field-error');
    }
    
	Origin.util.writeOverride('[Connection]', 'OverrideDownloadPath', offerId, $('#txt-add-game-local-override'));
    Origin.util.writeOverride('[Connection]', 'OverrideDownloadSyncPackagePath', offerId, $('#txt-add-game-sync-package'));
	Origin.util.writeOverride('[Connection]', 'OverrideVersion', offerId, $('#txt-add-game-local-version'));
	Origin.util.writeOverride('[Connection]', 'OverrideInstallFilename', offerId, $('#txt-add-game-installer-name'));
	Origin.util.writeOverride('[Connection]', 'OverrideExecuteRegistryKey', offerId, $('#txt-add-game-installation-location'));
	Origin.util.writeOverride('[Connection]', 'OverrideInstallRegistryKey', offerId, $('#txt-add-game-installation-location'));
	Origin.util.writeOverride('[Connection]', 'OverrideExecuteFilename', offerId, $('#txt-add-game-game-executable-name'));
    return true;
};

Origin.views.addGameOverride = new AddGameOverrideView();

})(jQuery);
