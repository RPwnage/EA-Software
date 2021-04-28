;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

$('#btn-view-client-log').on('click', function(event){
    window.originDeveloper.openClientLog();
});

$('#btn-view-bootstrap-log').on('click', function(event){
    window.originDeveloper.openBootstrapperLog();
});

$('#btn-launch-code-debug').on('click', function(event){
    window.originDeveloper.launchCodeRedemptionDebugger();
});

$('#btn-launch-telemetry-debug').on('click', function(event){
    window.originDeveloper.launchTelemetryLiveViewer();
});

$('#btn-launch-telemetry-debug').attr('disabled', window.clientSettings.readSetting('TelemetryXML') === false);

$('#btn-deactivate-odt').on('click', function(event){
    window.originDeveloper.deactivate();
});

})(jQuery);