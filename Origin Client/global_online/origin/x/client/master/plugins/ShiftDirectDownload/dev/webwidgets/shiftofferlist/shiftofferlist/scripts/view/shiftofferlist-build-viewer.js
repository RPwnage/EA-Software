;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

var BuildViewerView = function()
{
    $('#btn-apply-filter').on('click', function() {
        Origin.views.buildViewer.filterBuildList();
    });
    
    $('#btn-clear-filter').on('click', function() {
        $('#div-table-filters').find('select').val('');
        $('#div-table-filters').find('.origin-ux-drop-down-selection span').text($('#div-table-filters').find('select > option:selected').text());
        Origin.views.buildViewer.filterBuildList();
    });
    
    $('#btn-search-builds').on('click', function() {
        Origin.views.buildViewer.filterBuildList();
    });
    
    $('#btn-clear-search').on('click', function() {
        $('#txt-search-builds').val('');
        Origin.views.buildViewer.filterBuildList();
    });
    
    $('#btn-refresh-table').on('click', function() {
        Origin.views.buildViewer.refreshBuildList();
    });

    $('#btn-clear-active-build').on('click', function() {
        $('#active-build-id').html('<i>none</i>');
        $('#btn-clear-active-build').hide();
        window.originCommon.writeOverride('[Connection]', 'OverrideDownloadPath', 
            Origin.views.buildViewer.entitlement.productId, '');
        window.originCommon.writeOverride('[Connection]', 'OverrideVersion', 
            Origin.views.buildViewer.entitlement.productId, '');
        window.originCommon.applyChanges();
        shiftQuery.reloadOverrides();
    });

    $('#btn-download-build').on('click', function(event) {
        var view = Origin.views.buildViewer;
        var buildInfo = view.gameBuilds[view.selectedBuild];
        dcmtDownloadManager.startDownload(view.selectedBuild, buildInfo.version);
    });

    $('#btn-request-build').on('click', function(event) {
        var view = Origin.views.buildViewer;
        var buildId = view.selectedBuild;

        var requestBuild = function()
        {
            shiftQuery.placeDeliveryRequest(view.entitlement.productId, buildId);
            window.originCommon.alert('Your build has been requested.  Please wait for the build to be delivered to your Shift account folder.', 'Build Requested', 'info');
        };

        var query = shiftQuery.generateDownloadUrl(buildId);
        query.done.connect(function(status) {
            if (0 == status.downloadUrl.length)
            {
                requestBuild();
            }
        });
        query.fail.connect(function() {
            requestBuild();
        });

        var version = view.gameBuilds[buildId].version;
        var override = 'shift:' + buildId;
        $('#active-build-id').text(override);
        $('#btn-clear-active-build').show();
        window.originCommon.writeOverride('[Connection]', 'OverrideDownloadPath', 
            view.entitlement.productId, override);
        window.originCommon.writeOverride('[Connection]', 'OverrideVersion', 
            Origin.views.buildViewer.entitlement.productId, version);
        window.originCommon.applyChanges();
        shiftQuery.reloadOverrides();
    });

    $('#btn-cancel').on('click', function(event) {
        Origin.views.navbar.currentTab = "my-games";
    });

    function updateBuildStatus(status)
    {
        var text = Origin.views.buildViewer.getStatusText(status.status);
        var buildRow = $('#software-build-data').find('tr[build-id="' + status.buildId + '"]');
        buildRow.find('td.build-status').text(text).attr('build-status', status.status);
    };

    shiftQuery.deliveryStatusUpdated.connect(function(deliveryStatusList) {
        for (var i in deliveryStatusList)
        {
            updateBuildStatus(deliveryStatusList[i]);
        }
    });

    shiftQuery.deliveryRequestPlaced.connect(updateBuildStatus);

    var ellipsisCount = 0;
    var maxEllipsisCount = 5;
    setInterval(function() {
        var text = Origin.views.buildViewer.getStatusText('INPROGRESS') + Array(ellipsisCount + 1).join('.');
        $('#software-build-data').find('td[build-status="INPROGRESS"]').text(text);
        ellipsisCount = (ellipsisCount + 1) % (maxEllipsisCount + 1);
    }, 400);
};

BuildViewerView.prototype.init = function(id)
{
    var view = Origin.views.buildViewer;
    view.clearBuildViewer();

    var entitlement = window.entitlementManager.getEntitlementById(id);
    if (entitlement !== null)
    {
        view.entitlement = entitlement;
        view.renderGameDetails(entitlement);
    }
    Origin.util.badImageRender();

    var activeBuild = window.originCommon.readOverride('[Connection]',
        'OverrideDownloadPath', entitlement.productId);
    if (0 == activeBuild.length)
    {
        $('#active-build-id').html('<i>none</i>');
        $('#btn-clear-active-build').hide();
    }
    else
    {
        $('#active-build-id').text(activeBuild);
        $('#btn-clear-active-build').show();
    }
};

BuildViewerView.prototype.clearBuildViewer = function()
{
    $('#software-build-data').find('tbody').html('');

    $('#content-id').html('');
    $('#offer-id').html('');
    $('#item-id').html('');
    $('#master-title-id').html('');

    $('#game-title-header').html('');
    $('#build-viewer').find('#game-boxart > img').attr('src', Origin.model.constants.DEFAULT_IMAGE_PATH);
    
    $('#div-table-filters').find('select').val('');
    $('#div-table-filters').find('.origin-ux-drop-down-selection span').text($('#div-table-filters').find('select > option:selected').text());
    $('#txt-search-builds').val('');
    $('#btn-request-build').prop('disabled', true);
    $('#btn-download-build').prop('disabled', true);

    Origin.views.buildViewer.selectedBuild = '';
    Origin.views.buildViewer.gameBuilds = {};
}

BuildViewerView.prototype.renderGameDetails = function(entitlement)
{
    console.info("Showing build viewer for: " + entitlement.productId);
    console.info(entitlement);

    $('#game-title-header').html(entitlement.title);
    $('#content-id').html(entitlement.contentId);
    $('#offer-id').html(entitlement.productId);
    $('#item-id').html(entitlement.expansionId);
    $('#master-title-id').html(entitlement.masterTitleId);

    $('#game-boxart > img').attr('src', entitlement.boxartUrls.length > 0 ? entitlement.boxartUrls[0] : Origin.model.constants.DEFAULT_IMAGE_PATH);
    $('#game-boxart > img').attr('title', entitlement.title);

    Origin.views.buildViewer.refreshBuildList();
};

BuildViewerView.prototype.refreshBuildList = function()
{
    $(document.body).addClass('wait');

    var entitlement = Origin.views.buildViewer.entitlement;
    var query = shiftQuery.getAvailableBuilds(entitlement.productId);
    query.done.connect(function(buildList) {
        $('#permission-denied').hide();
        $('#base-game').show();

        // Convert from JS Array to JS Object
        var builds = {};
        for (var idx in buildList)
        {
            var build = buildList[idx];
            builds[build.buildId] = build;
        }

        Origin.views.buildViewer.gameBuilds = builds;
        
        // Preserve any filters that were active pre-refresh.
        Origin.views.buildViewer.filterBuildList();
    });

    query.fail.connect(function() {
        var err = query.getError();
        if ("PERMISSION_DENIED" === err) {
            $('#base-game').hide();
            $('#permission-denied').show();
        }
    });

    query.always.connect(function() {
        $(document.body).removeClass('wait');
    });
};

BuildViewerView.prototype.filterBuildList = function()
{
    var buildType = $('#div-table-filters').find('select').val();
    var search = $('#txt-search-builds').val();

    var filteredBuilds = $.extend(true, {}, Origin.views.buildViewer.gameBuilds);
    
    if(buildType !== '')
    {
        // Remove any builds that do not match the current filter (if any)
        $.each(filteredBuilds, function(idx, build)
        {
            if (build.buildTypes.indexOf(buildType) === -1)
            {
                delete filteredBuilds[build.buildId];
            }
        });
    }
    
    if(search !== '')
    {
        // Remove any builds that do not match the current search value (if any)
        $.each(filteredBuilds, function(idx, build)
        {
            if (build.buildId.toLowerCase().indexOf(search.toLowerCase()) === -1 && 
                build.version.toLowerCase().indexOf(search.toLowerCase()) === -1)
            {
                delete filteredBuilds[build.buildId];
            }
        });
    }

    Origin.views.buildViewer.renderSoftwareBuilds(filteredBuilds);
};

BuildViewerView.prototype.renderSoftwareBuilds = function(gameBuilds)
{
    var view = Origin.views.buildViewer;
    var tbody = '';
    var count = 0;
    $.each(gameBuilds, function(idx, build)
    {
        tbody += '<tr class="game-build-row" build-id="' + build.buildId + '"><td class="build-id">' + build.buildId + '</td>';
        tbody += '<td class="build-version">' + build.version + '</td>';
        tbody += '<td class="build-status" build-status="' + build.status + '">' + view.getStatusText(build.status) + '</td></tr>';
        
        ++count;
        
        // Returning false breaks the loop.  Break after we've added the maximum number of builds.
        return count < Origin.model.constants.MAX_BUILDS_TO_SHOW;
    });

    $('#software-build-data').find('tbody').html(tbody);

    $('#software-build-data > table > tbody > tr').each(function()
    {
        $(this).on('click', function(event)
        {
            var table = $('#software-build-data');
            var oldBuildId = view.selectedBuild;
            var newBuildId = $(this).attr('build-id');
            if (oldBuildId != newBuildId)
            {
                var newBuild = view.gameBuilds[newBuildId];

                if (oldBuildId != '')
                {
                    table.find('tr[build-id="' + oldBuildId + '"]').removeClass('selected-build');
                }
                table.find('tr[build-id="' + newBuildId + '"]').addClass('selected-build');
                view.selectedBuild = newBuildId;

                $('#btn-request-build').prop('disabled', false);
                $('#btn-download-build').prop('disabled', false);
            }
        });
    });
    
    // Unforunately, the only way to have a scrollable table with a fixed header
    // is to make the thead>tr and tbody display:block.  This puts the columns
    // out of alignment, so we have to manually size them.  When a scrollbar appears
    // or disappears, we need to account for this by twiddling our manual size.
    var scrollbarVisible = false;
    var $tbody = $("#software-build-data > table > tbody");
    if($tbody && $tbody[0].scrollHeight > $tbody.innerHeight())
    {
        scrollbarVisible = true;
    }
    
    $('#software-build-data > table > tbody > tr > td.build-status').each(function()
    {
        if(scrollbarVisible)
            $(this).addClass("scrollbar-visible");
        else
            $(this).removeClass("scrollbar-visible");
    });
};

BuildViewerView.prototype.getStatusText = function(status)
{
    if ('INPROGRESS' === status)
    {
        return 'Preparing build';
    }
    else if ('COMPLETED' === status)
    {
        return 'Ready to download';
    }
    return status;
};

Origin.views.buildViewer = new BuildViewerView();

})(jQuery);
