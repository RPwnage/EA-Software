var sonar = null;
var userId = null;
var channelId = null;

function getToken(userId, userDescription, channelId, channelDesc, callback) {
    var paramString = "";
    var params = [];

    if (userDescription !== undefined && userDescription != null) { params.push('udc=' + userDescription); }
    if (channelId !== undefined && channelId != null) { params.push('cid=' + channelId); }
    if (channelDesc !== undefined && channelDesc != null) { params.push('cdc=' + channelDesc); }

    if (params.length > 0)
        paramString = "?" + params.join('&');

    $.getJSON("/api/default/users/" + userId + "/control-token" + paramString, callback);
}

function joinChannel(userId, channelId, channelDesc, location, callback) {
    var paramString = "";
    var params = [];

    if (channelDesc !== undefined && channelDesc != null) { params.push('channelDesc=' + channelDesc); }
    if (location !== undefined && location != null) { params.push('location=' + location); }

    if (params.length > 0)
        paramString = "?" + params.join('&');

    $.ajax({url: "/api/default/users/" + userId + "/channel" + paramString, type: 'PUT', dataType: 'json', data: '"' + channelId + '"', success: callback, contentType: 'application/json'});
}

function partChannel(userId, channelId, reasonType, reasonDesc, callback) {
    var paramString = "";
    var params = [];

    if (reasonType !== undefined && reasonType != null) { params.push('reasonType=' + reasonType); }
    if (reasonDesc !== undefined && reasonDesc != null) { params.push('reasonDesc=' + reasonDesc); }

    if (params.length > 0)
        paramString = "?" + params.join('&');

    $.ajax({url: "/api/default/users/" + userId + "/channel/" + channelId + paramString, type: 'DELETE', dataType: 'json', success: callback, contentType: 'application/json'});
}

function setupUi() {
    $("#playback-volume-slider").slider({
        min: 0, max: 1, step: 0.01,
        orientation: "vertical",
        range: "min",
        value: sonar.playbackVolume(),
        disabled: false,
        change: function(event, ui) { sonar.playbackVolume(ui.value); }
    });

    $("#capture-volume-slider").slider({
        min: 0, max: 1, step: 0.01,
        orientation: "vertical",
        range: "min",
        value: sonar.captureVolume(),
        disabled: false,
        change: function(event, ui) { sonar.captureVolume(ui.value); }
    });

    $("#voice-sensitivity-slider").slider({
        min: 0, max: 1, step: 0.01,
        orientation: "vertical",
        range: "min",
        value: sonar.voiceSensitivity(),
        disabled: false,
        change: function(event, ui) { sonar.voiceSensitivity(ui.value); }
    });

    $('#mute-playback-button').removeAttr('disabled');
    $('#mute-capture-button').removeAttr('disabled');
    $('#capture-mode-button').removeAttr('disabled');

    $('#playback-muted').attr('checked', sonar.playbackMuted());
    $('#capture-muted').attr('checked', sonar.captureMuted());
    $('#audio-loopback').attr('checked', sonar.audioLoopback());
    $('#echo-cancellation').attr('checked', sonar.echoCancellation());


    $("input[name='capture-mode']").removeAttr('checked');
    $("input[value='" + sonar.captureMode() + "']").attr('checked', true);
    $("#capture-mode").button('refresh');
    $("input[name='capture-mode']").button("refresh");

    $('#setup-client').hide();
    $('#client-main').show();

    $("#capture-mode").buttonset('enable');
    $('#playback-muted').removeAttr('disabled');
    $('#capture-muted').removeAttr('disabled');
    $('#audio-loopback').removeAttr('disabled');
    $('#echo-cancellation').removeAttr('disabled');
    $('#push-to-talk-key').removeAttr('disabled');
    $('#push-to-talk-key').val(sonar.pushToTalkKey());
    $('#voice-activation-mode').removeAttr('disabled');
    $('#push-to-talk-mode').removeAttr('disabled');
    $('#continuous-mode').removeAttr('disabled');


    $('#playback-device').removeAttr('disabled');
    $('#capture-device').removeAttr('disabled');

    $('#playback-device').empty();
    var devices = sonar.playbackDevices();
    var currentDevice = sonar.playbackDevice();
    var selected = '';
    for (var i in devices) {
        selected = '';
        if (devices[i].deviceId == currentDevice) { selected = ' selected="selected"'; }
        $('#playback-device').append('<option value="' + devices[i].deviceId + '"' + selected + '>' + devices[i].deviceName + '</option>');
    }

    $('#capture-device').empty();
    devices = sonar.captureDevices();
    currentDevice = sonar.captureDevice();
    for (var k in devices) {
        selected = '';
        if (devices[k].deviceId == currentDevice) { selected = ' selected="selected"'; }
        $('#capture-device').append('<option value="' + devices[k].deviceId + '"' + selected + '>' + devices[k].deviceName + '</option>');
    }


    var s = sonar.captureMode() == 'pushtotalk' ? 'disable' : 'enable';
    $('#voice-sensitivity-slider').slider(s);

    redrawUsers();
}

function redrawUsers() {
    $('#talkers').empty();
    var users = sonar.usersInChannel();

    if (users.length == 0) {
        $('#talkers').append('<tr><td colspan="2">Not in a channel</td></tr>');
        return;
    }

    for (var i in users) {
        var u = users[i];
        var state = '';
        if (u.talking) state = '_on';
        if (u.muted) state = '_disabled';

        var img = '<td style="width: 30px;"><img class="speaker" src="img/icon_speaker' + state + '.png" alt="' + u.userId + '" style="padding-top: 6px; margin-bottom: 0px;"></td>';
        $('#talkers').append('<tr>' + img + '<td>' + u.userDescription + '</td></tr>');
    }
}

function defaultInstanceName() {
    var hostname = window.location.hostname;
    var validIp = /^(([1-9][0-9]{0,2})|0)\.(([1-9][0-9]{0,2})|0)\.(([1-9][0-9]{0,2})|0)\.(([1-9][0-9]{0,2})|0)$/;
    if (validIp.test(hostname))
        return hostname;

    var s = hostname.split('.');
    hostname = s[s.length - 2] + "." + s[s.length - 1];
    return hostname;
}

$(document).ready(function () {
    var readAudio = false;
    var captureVolume = $('#capture-volume .meter');
    var max = 0;
    var userId = "defaultuser";
    channelId = "Test Channel";
    var channelDesc = "My channel description";

    if ($.cookie("userId") == null)
        $.cookie("userId", defaultInstanceName());

    if ($.cookie("channelId") == null)
        $.cookie("channelId", 'Test Channel');

    if ($.cookie("instanceName") == null)
        $.cookie("instanceName", defaultInstanceName());

    $('#user-id').val($.cookie("userId"));
    $('#user-id').change(function (e) {
        $.cookie("userId", $('#user-id').val());
    });

    $('#channel-id').val($.cookie("channelId"));
    $('#channel-id').change(function (e) {
        $.cookie("channelId", $('#channel-id').val());
    });

    $('#instance-name').val($.cookie("instanceName"));
    $('#instance-name').change(function (e) {
        $.cookie("instanceName", $('#instance-name').val());
    });

    $.get('/client-filename', function (data) {
        $('#download-link').attr('href', '/' + data);
    });

    $("#playback-volume-slider").slider({orientation: "vertical", value: 0, disabled: true, range: "min"});
    $("#capture-volume-slider").slider({orientation: "vertical", value: 0, disabled: true, range: "min"});
    $("#voice-sensitivity-slider").slider({orientation: "vertical", value: 0, disabled: true, range: "min"});

    $('#connect-button').click(function () {
        $('#connect-status').hide();
        $('#setup-loader').show();
        console.log("Desc", $('#user-description').val());
        //var userId = $('#user-id').val();
        userId = Math.floor(Math.random()*1000000) + "";
        getToken(userId, $('#user-description').val(), null, null, function (token) {
            console.log('Token', token);
            try {
                sonar = new SonarVoice(token, {instanceName: $('#instance-name').val()});
            } catch (e) {
                $('#setup-loader').hide();
                //$('#connect-status').html(e.message).show();
                $('#connect-status').show();
                $('#connect-status div').html(e.message);
                return;
            }
            sonar.bind('*', function (e) { console.log('*', e.name, e); });
            sonar.bind('playbackmute', function (e) { $('#playback-muted').attr('checked', e.muted); });
            sonar.bind('capturemute', function (e) { $('#capture-muted').attr('checked', e.muted); });
            sonar.bind('audioloopback', function (e) { $('#audio-loopback').attr('checked', e.enabled); });
            sonar.bind('connect', redrawUsers);
            sonar.bind('disconnect', redrawUsers);
            sonar.bind('channelusertalk', redrawUsers);
            sonar.bind('channelusermute', redrawUsers);
            sonar.bind('channeluserjoin', redrawUsers);
            sonar.bind('channeluserpart', redrawUsers);
            sonar.bind('hotkey', function (e) { if (e.type == 'ptt') $('#push-to-talk-key').val(e.key); });


            sonar.bind('connect', function (e) {
                channelId = e.channelId;
                $('#sidebar-users h2').html('Users (in ' + e.channelDescription + ')');
                $('#join-loader').hide();
                $('#client-main').hide();
                $('#client-channel').show();
            });
            //sonar.bind('audio', function (e) { console.log('audio', e); });
            setupUi();
        });
    });

    $('#join-button').click(function () {

        $('#join-loader').show();
        joinChannel(userId, $('#channel-id').val(), $('#channel-id').val(), null, function () {
            console.log('Channel join requested. Waiting for server to join us to correct channel...');
        });
    });

    $('#mute-playback-button').click(function (e) { sonar.togglePlaybackMuted(); });
    $('#mute-capture-button').click(function () { sonar.toggleCaptureMuted(); });
    $('#capture-mode-button').click(function () { sonar.toggleVoiceActivation(); });
    $('#push-to-talk-key').dblclick(function () {
        setTimeout(function () {
            sonar.recordPushToTalkKey();
            $('#push-to-talk-key').val('Recording keystroke...');
        }, 200);
    });

    $("#show-commands").click(function () { $('#command-list').show(); });
    //$("#capture-mode").buttonset();
    //$("#capture-mode").buttonset('disable');
    $("input[name='capture-mode']").live('change', function() {
        if ($("input[name='capture-mode']:checked").val())
            var mode = sonar.captureMode($(this).val());
            var s = mode == 'pushtotalk' ? 'disable' : 'enable';
            $('#voice-sensitivity-slider').slider(s);
    });

    $('#playback-device').live('change', function(e) { sonar.playbackDevice(this.options[this.selectedIndex].value); });
    $('#capture-device').live('change', function(e) { sonar.captureDevice(this.options[this.selectedIndex].value); });

    $('#playback-muted').live('change', function() { sonar.playbackMuted($(this).attr('checked')); });
    $('#capture-muted').live('change', function() { sonar.captureMuted($(this).attr('checked')); });
    $('#audio-loopback').live('change', function() { sonar.audioLoopback($(this).attr('checked')); });
    $('#echo-cancellation').live('change', function() { sonar.echoCancellation($(this).attr('checked')); });



    $('table#talkers img.speaker').live('click', function (e) {
        var userId = $(this).attr('alt');
        sonar.toggleUserMuted(userId);
    });

    $('table#talkers img.speaker').live('hover', function (e) {
        $(this).addClass('speaker-hover');
    }, function (e) {
        $(this).removeClass('speaker-hover');
    });

    $('#join-another-channel').click(function () {

        //$('#sidebar-users h2').html('Users');
        //$('#client-main').show();
        //$('#client-channel').hide();

        partChannel(userId, channelId, null, null, function () {
            //sonar.partChannel();
            $('#sidebar-users h2').html('Users');
            $('#client-main').show();
            $('#client-channel').hide();
        });
    });
});