;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var OigBraodcastView = function()
{
    var $disconnectLink = $('.broadcast-connection');
    $('.broadcast-header .otktitle-2').append(' (' + tr('ebisu_client_beta') + ')');
    $disconnectLink.text(tr('ebisu_client_disconnect'));

    $('.broadcast-cta .otkbtn-primary').click(function() {
        if(broadcast.isBroadcasting)
        {
            broadcast.attemptBroadcastStop();
        }
        else
        {
            broadcast.attemptBroadcastStart();
        }
    });
    
    $disconnectLink.click(function() {
        broadcast.disconnectAccount();
    });
    
    // TODO: check if we need to add keyboard events too for left and right slider
    $('.otkslider > input').on('show mousemove mouseup', function(){
        $('.otkslider-track[for = "' + $(this).attr('id') + '"]').css('width', $(this).val().toString() + '%');
    });

    // Click on Tab - depends on section
    $('.otktab').on('click', function(event){
        event.preventDefault();
        $(this).siblings().removeClass('otktab-active');
        $(this).addClass('otktab-active');
        $('.otktab-content').removeClass('otktab-content-active');
        $('#'+$(this).data('section')).addClass('otktab-content-active');
    });
    
    
    $("#slider-mic").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastMicVolume", this.value);
    });
    
    $("#slider-game").on('change', function(evt) {
        clientSettings.writeSetting("GameMicVolume", this.value);
    });

    $("#broadcast-autostart").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastAutoStart", this.checked);
    });
    
    $("#mute-mic").on('change', function(evt) {
        $('.voice-slider').attr('disabled', this.checked);
        clientSettings.writeSetting("BroadcastMicMute", this.checked);
    });
    
    $("#mute-game").on('change', function(evt) {
        $('.game-slider').attr('disabled', this.checked);
        clientSettings.writeSetting("GameMicMute", this.checked);
    });
    
    $("input.broadcast-gamename").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastGameName", this.value);
    });
    
    $("#optimized-broadcasting").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastUseOptimizedEncoder", this.checked);
    });
    
    $("#broadcasting-status").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastShowViewersNum", this.checked);
    });
    
    $(".broadcast-resolution-options-select").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastResolution", this.value);
    });
    
    $(".broadcast-framerate-options-select").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastFramerate", this.value);
    });

    $(".broadcast-picture-quality-options-select").on('change', function(evt) {
        clientSettings.writeSetting("BroadcastQuality", this.value);
    });

    this.onUpdateSettings();
    this.onBroadcastStatusChange();
    this.generateBanners();
    this.onBroadcastOptEncoderAvailable(broadcast.isBroadcastOptEncoderAvailable);
    clientSettings.updateSettings.connect($.proxy(this, 'onUpdateSettings'));
    broadcast.broadcastStarted.connect($.proxy(this, 'onBroadcastStatusChange'));
    broadcast.broadcastStopped.connect($.proxy(this, 'onBroadcastStatusChange'));
    broadcast.broadcastStatusChanged.connect($.proxy(this, 'onBroadcastStatusChange'));
    broadcast.broadcastingStateChanged.connect($.proxy(this, 'onBroadcastStatusChange'));
    broadcast.broadcastUserNameChanged.connect($.proxy(this, 'onBroadcastUserNameChanged'));
    broadcast.broadcastChannelChanged.connect($.proxy(this, 'onBroadcastChannelChanged'));
    broadcast.broadcastOptEncoderAvailable.connect($.proxy(this, 'onBroadcastOptEncoderAvailable'));
    // Initialize the slider background position
    $('.otkslider-track[for = "' + $('.voice-slider > input').attr('id') + '"]').css('width', $('.voice-slider > input').val().toString() + '%');
    $('.otkslider-track[for = "' + $('.game-slider > input').attr('id') + '"]').css('width', $('.game-slider > input').val().toString() + '%');
};

OigBraodcastView.prototype.generateBanners = function()
{
    var env = clientSettings.readSetting("EnvironmentName");
    var inProd = env === 'production';
    var isArchiving = clientSettings.readSetting("BroadcastSaveVideo");
    var $bannerContainer = $('.banner-container');
    var $noticeElement;
    var $archiveErrorLink;

    $bannerContainer.html('');
    if(!inProd)
    {
        $noticeElement = $('.notice-to-clone').clone();
        $noticeElement.removeClass('notice-to-clone');
        $noticeElement.find(".notice-message").html(tr('ebisu_client_critical_warning_broadcast'));
        $bannerContainer.append($noticeElement);
        $noticeElement.show();
    }
    
    if(!isArchiving)
    {
        $noticeElement = $('.notice-to-clone').clone();
        $noticeElement.removeClass('notice-to-clone');
        $noticeElement.find(".notice-message").html(tr('ebisu_client_no_twitch_archiving').replace("settings-archive", "#"));
        $archiveErrorLink = $noticeElement.find("a");
        $archiveErrorLink.addClass("otka");
        $archiveErrorLink.click(function() {
            clientNavigation.launchExternalBrowser(clientSettings.readSetting("BroadcastSaveVideoFix"))
        });
        $bannerContainer.append($noticeElement);
        $noticeElement.show();
    }
}

OigBraodcastView.prototype.onBroadcastUserNameChanged = function(name)
{
    if(name)
    {
        $('.twitch-account-info .twitch-label').html(tr('ebisu_client_twitch_connected_to_twitch_as_user').replace("%1", name));
    }
    else
    {
        $('.twitch-account-info .twitch-label').text(tr('ebisu_client_disconnected_twitch'));
    }
}

OigBraodcastView.prototype.onBroadcastChannelChanged = function(channel)
{
    var userChannelElement;
    if(channel)
    {
        userChannelElement = '<a href="#" class="otka twitch-username" onclick="clientNavigation.launchExternalBrowser(\'' + channel + '\')">' + channel + '</a>';
        $('.oig-broadcast-header-message').html(tr('ebisu_client_broadcast_gameplay_text_to').replace("%1", userChannelElement));
    }
    else
    {
        $('.oig-broadcast-header-message').html('');
    }
}

OigBraodcastView.prototype.onBroadcastOptEncoderAvailable = function(enable)
{
    $('#optimized-broadcasting').toggle(enable);
}

OigBraodcastView.prototype.onBroadcastStatusChange = function()
{
    this.onBroadcastUserNameChanged(broadcast.broadcastUserName);
    this.onBroadcastChannelChanged(broadcast.broadcastChannel);
    var $primaryBtn = $('.broadcast-cta .otkbtn-primary');
    if(broadcast.isBroadcastingPending)
    {
        $('.otktab-content').attr('disabled', true);
        $primaryBtn.attr('disabled', true);
        $primaryBtn.text(tr('ebisu_client_video_broadcast_spinner_text'));
        $primaryBtn.addClass('otkform-group-loading');
    }
    else if(broadcast.isBroadcasting)
    {
        $('.otktab-content').attr('disabled', true);
        $primaryBtn.attr('disabled', false);
        $primaryBtn.text(tr('ebisu_client_stop_broadcast'));
        $primaryBtn.removeClass('otkform-group-loading');
    }
    else
    {
        $('.otktab-content').attr('disabled', false);
        $primaryBtn.attr('disabled', false);
        $primaryBtn.text(tr('ebisu_client_start_broadcast'));
        $primaryBtn.removeClass('otkform-group-loading');
    }
}

OigBraodcastView.prototype.onUpdateSettings = function(name, value)
{
    var micMuted = false;
    var gameMuted = false;
    switch(name)
    {
        case 'BroadcastSaveVideo':
            this.generateBanners();
            break;
        case 'BroadcastMicVolume':
            $("#slider-mic").val(value);
            break;
        case 'GameMicVolume':
            $("#slider-game").val(value);
            break;
        case 'BroadcastGameName':
            $(".broadcast-gamename").val(value);
            break;
        case 'BroadcastMicMute':
            $('.voice-slider').attr('disabled', value);
            $("#mute-mic").prop("checked", value);
            break;
        case 'GameMicMute':
            $('.game-slider').attr('disabled', value);
            $("#mute-game").prop("checked", value);
            break;
        case 'BroadcastResolution':
            $(".broadcast-resolution-label").text($(".broadcast-resolution-options-select option[value='" + value + "']").text());
            break;
        case 'BroadcastFramerate':
            $(".broadcast-framerate-label").text($(".broadcast-framerate-options-select option[value='" + value + "']").text());
            break;
        case 'BroadcastQuality':
            $(".broadcast-picture-quality-label").text($(".broadcast-picture-quality-options-select option[value='" + value + "']").text());
            break;
        case 'BroadcastAutoStart':
            $("#broadcast-autostart").val(value);
            break;
        case 'BroadcastUseOptimizedEncoder':
            $("#optimized-broadcasting").val(value);
            break;
        case 'BroadcastShowViewersNum':
            $("#broadcasting-status").val(value);
            break;
        default:
        {
            $(".broadcast-gamename").val(clientSettings.readSetting("BroadcastGameName"))
            micMuted = clientSettings.readSetting("BroadcastMicMute");
            gameMuted = clientSettings.readSetting("GameMicMute");
            $("#slider-mic").val(clientSettings.readSetting("BroadcastMicVolume"));
            $("#slider-game").val(clientSettings.readSetting("GameMicVolume"));
            $("#mute-mic").prop("checked", micMuted);
            $('.voice-slider').attr('disabled', micMuted);
            $('.game-slider').attr('disabled', gameMuted);
            $("#mute-game").prop("checked", gameMuted);
            $(".broadcast-resolution-label").text($(".broadcast-resolution-options-select option[value='" + clientSettings.readSetting("BroadcastResolution") + "']").text());
            $(".broadcast-framerate-label").text($(".broadcast-framerate-options-select option[value='" + clientSettings.readSetting("BroadcastFramerate") + "']").text());
            $(".broadcast-picture-quality-label").text($(".broadcast-picture-quality-options-select option[value='" + clientSettings.readSetting("BroadcastQuality") + "']").text());
            $("#broadcast-autostart").prop("checked", clientSettings.readSetting("BroadcastAutoStart"));
            $("#optimized-broadcasting").prop("checked", clientSettings.readSetting("BroadcastUseOptimizedEncoder"));
            $("#broadcasting-status").prop("checked", clientSettings.readSetting("BroadcastShowViewersNum"));
            break;
        }
    }
};

// Expose to the world
Origin.views.oigBroadcast = new OigBraodcastView();

})(jQuery);