window.originTranslator = {
    fallbackStrings: {
        "ebisu_client_preparing_game": "Preparing",
        "ebisu_client_pausing_game": "Pausing %1",
        "ebisu_client_download_game_paused": "%1 Paused",
        "ebisu_client_canceling_game": "Cancelling %1",
        "ebisu_client_finalizing_game": "Finalizing %1",
        "ebisu_client_cloud_sync": "cloud sync",
        "ebisu_client_cloud_game_card_status": "Syncing Cloud Data",
        "ebisu_client_update": "Update",
        "ebisu_client_updating": "Updating",
        "ebisu_client_preloading": "Preloading",
        "ebisu_client_downloading": "Downloading",
        "ebisu_client_install": "Install",
        "ebisu_client_installing": "Installing",
        "ebisu_client_repair": "Repair",
        "ebisu_client_repairing": "Repairing",
        "ebisu_client_unpack": "Unpack",
        "ebisu_client_unpacking": "Unpacking",
        "ebisu_client_resuming": "Resuming",
        "ebisu_client_colon_placement": "%0: %1",
        "ebisu_client_percent_notation": "%1%",
        "ebisu_client_ellipsis": "…",
        "ebisu_client_byte_notation": "%1 B",
        "ebisu_client_mbyte_notation": "%1 KB",
        "ebisu_client_kbyte_notation": "%1 MB",
        "ebisu_client_gbyte_notation": "%1 GB",
        "application_name": "Origin",
        "cc_caption_time_remaining": "%1 Remaining",
        "percent_complete": "%1 Complete",
        "ebisu_client_preload_noun": "Preload",
        "ebisu_client_download": "Download",
        "ebisu_client_ready_to_install" : "Ready to Install",
        "ebisu_client_installation_complete_play_now" : "Play %1 now",
        "ebisu_client_pause_game" : "Pause %1",
        "ebisu_client_cancel_game" : "Cancel %1",
        "ebisu_client_resume_game" : "Resume %1",
        "ebisu_client_all_downloads_complete": "There are no active tasks at this time",
        "seconds": "sec",
        "ebisu_client_verifying_game_files" : "Verifying game files",
        "ebisu_client_notranslate_show_details" : "Show Details",
        "ebisu_client_play" : "Play",
        "ebisu_client_full_game" : "Full Game",
        "ebisu_client_game_dlc" : "%1 DLC",
        "ebisu_client_waiting" : "Waiting",
        "ebisu_client_network_speed" : "Network Speed"
    },

    translate: function (string_id) {
        var fallbackStrings = originTranslator.fallbackStrings;

        if (string_id in fallbackStrings) {
            return fallbackStrings[string_id];
        }

        return string_id;
    }
}

window.tr = function (string_id) {
    return originTranslator.translate(string_id);
}
