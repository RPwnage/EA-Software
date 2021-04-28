$(function() {
    "use strict";
    
    window.progressBar = new StandardProgressBar($('.origin-progressbar'));
    
    $('#btn-cancel').on('click', function(event) {
        if(window.dcmtDownload !== undefined)
        {
            window.dcmtDownload.cancel();
        }
    });

    function formatBytes(bytes)
    {
        var KB = 1024;
        var MB = 1048576;
        var GB = 1073741824;
        if (bytes < KB)
        {
            return '<b>' + bytes.toFixed(0) + '</b> bytes';
        }
        else if (bytes < MB)
        {
            return '<b>' + (bytes / KB).toFixed(2) + '</b> KB';
        }
        else if (bytes < GB)
        {
            return '<b>' + (bytes / MB).toFixed(2) + '</b> MB';
        }
        else
        {
            return '<b>' + (bytes / GB).toFixed(2) + '</b> GB';
        }
    };
    
    $('#file-name').html(window.dcmtDownload.fileName);
    $('#download-state').html('Generating download URL...');
    
    // Hide these since we do not have this information until the download has made progress.
    $('#transfer-rate').hide();
    $('#bytes-per-second').hide();
    $('#time-remaining').hide();
    
    // Indeterminate until download starts.
    window.progressBar.setIndeterminate();

    window.dcmtDownload.downloadProgress.connect(function(bytesDownloaded, totalBytes, bytesPerSecond, estimatedSecondsRemaining) {
        var transferRate = formatBytes(bytesDownloaded) + ' / ' + formatBytes(totalBytes);
        $('#transfer-rate').html(transferRate);
        $('#transfer-rate').show();
        
        if(bytesPerSecond !== -1)
        {
            var bytesPerSecond = formatBytes(bytesPerSecond);
            $('#bytes-per-second').html(bytesPerSecond + '/sec');
            $('#bytes-per-second').show();
        }
        else
        {
            $('#bytes-per-second').hide();
        }
        
        // Sometimes the downloader will report -1 seconds remaining.
        // When that happens, show 0 seconds remaining.
        if(estimatedSecondsRemaining === -1)
        {
            estimatedSecondsRemaining = 0;
        }
        
        var timeRemaining = '<b>' + dateFormat.formatShortInterval(estimatedSecondsRemaining) + '</b> Remaining';
        $('#time-remaining').html(timeRemaining);
        $('#time-remaining').show();
        
        var progress = bytesDownloaded / totalBytes;
        window.progressBar.setProgress(progress);
        window.progressBar.setLabel((progress * 100).toFixed(0) + '% Complete');
    });
    
    window.dcmtDownload.deliveryRequested.connect(function(buildId) {
        console.log('Delivery requested: ' + buildId);
        window.progressBar.setIndeterminate();
        window.progressBar.setLabel('');
        
        $('#banner-box').show();
        $('#download-state').html('Waiting for build delivery...');
        $('#transfer-rate').hide();
        $('#bytes-per-second').hide();
        $('#time-remaining').hide();
        $('#btn-cancel').show();
    });
    
    window.dcmtDownload.started.connect(function(buildId) {
        console.log('Download started: ' + buildId);
        window.progressBar.setProgress(0);
        window.progressBar.setLabel('0% Complete');
        
        $('#banner-box').hide();
        $('#download-state').html('Downloading from Shift...');
        $('#transfer-rate').hide();
        $('#bytes-per-second').hide();
        $('#time-remaining').hide();
        $('#btn-cancel').show();
    });
    
    window.dcmtDownload.finished.connect(function(buildId) {
        console.log('Download complete: ' + buildId);
        window.progressBar.setComplete();
        window.progressBar.setLabel('Download Complete');
        
        $('#banner-box').hide();
        $('#download-state').html('');
        $('#transfer-rate').show();
        $('#bytes-per-second').hide();
        $('#time-remaining').hide();
        $('#btn-cancel').hide();
    });
    
    window.dcmtDownload.error.connect(function(buildId) {
        console.log('Download failed: ' + buildId);
        window.progressBar.setComplete();
        window.progressBar.setLabel('Download Error');
        
        $('#banner-box').hide();
        $('#download-state').html('');
        $('#transfer-rate').hide();
        $('#bytes-per-second').hide();
        $('#time-remaining').hide();
        $('#btn-cancel').hide();
    });
    
    window.dcmtDownload.canceled.connect(function(buildId) {
        console.log('Download canceled: ' + buildId);
        window.progressBar.setComplete();
        window.progressBar.setLabel('Download Canceled');
        
        $('#banner-box').hide();
        $('#download-state').html('');
        $('#transfer-rate').hide();
        $('#bytes-per-second').hide();
        $('#time-remaining').hide();
        $('#btn-cancel').hide();
    });
});
