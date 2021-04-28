package com.ea.originx.automation.lib.macroaction;

/**
 * Class for download options.
 *
 * @author palui
 */
public final class DownloadOptions {

    // GUI options
    protected boolean desktopShortcut;
    protected boolean startMenuShortcut;
    protected boolean uncheckExtraContent;
    protected String newInstallLocation;

    // Flow options
    /**
     * Enum for Download Trigger
     */
    public enum DownloadTrigger {

        INSTALL_FROM_DISC, DOWNLOAD_FROM_GAME_TILE, DOWNLOAD_FROM_GAME_SLIDEOUT_COG_WHEEL
    }

    protected DownloadTrigger downloadTrigger;

    protected boolean stopAtEULA;

    /**
     * Constructor to set the default options.
     */
    public DownloadOptions() {

        // UI options
        desktopShortcut = true;
        startMenuShortcut = true;
        uncheckExtraContent = true;

        // Flow options
        downloadTrigger = DownloadTrigger.DOWNLOAD_FROM_GAME_TILE; //Set the default option as Download from Game Tile
        stopAtEULA = false;
        newInstallLocation = "";
    }

    /**
     * Checks the 'Desktop Shortcut' checkbox for download and install UI.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public DownloadOptions setDesktopShortcut(boolean option) {
        desktopShortcut = option;
        return this;
    }

    /**
     * Checks the 'Start Menu Shortcut' checkbox for download and install UI.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public DownloadOptions setStartMenuShortcut(Boolean option) {
        startMenuShortcut = option;
        return this;
    }

    /**
     * Un-check the 'Extra Content' checkbox for download and install UI.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public DownloadOptions setUncheckExtraContent(boolean option) {
        uncheckExtraContent = option;
        return this;
    }

    /**
     * Sets the DownloadTrigger option.
     *
     * @param downloadTrigger Option to set
     * @return This object (to allow chain setting)
     */
    public DownloadOptions setDownloadTrigger(DownloadTrigger downloadTrigger) {
        this.downloadTrigger = downloadTrigger;
        return this;
    }

    /**
     * Sets the 'Stop at EULA' option. If true, stop when the EULA dialog appears.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public DownloadOptions setStopAtEULA(boolean option) {
        stopAtEULA = option;
        return this;
    }
    
    /**
     * Sets the 'Install Location'
     *
     * @param newLocation New location to install game
     * @return This object (to allow chain setting)
     */
    public DownloadOptions setChangeDownloadLocation(String newLocation) {
        newInstallLocation = newLocation;
        return this;
    }

    /**
     * Returns String representation of this DownloadOptions.
     *
     * @return String representing this DownloadOptions object
     */
    @Override
    public String toString() {
        return "DownloadOptions{"
                + "desktopShortcut=" + desktopShortcut
                + ", startMenuShortcut=" + startMenuShortcut
                + ", uncheckExtraContent=" + uncheckExtraContent
                + ", downloadTrigger=" + downloadTrigger
                + ", stopAtEULA=" + stopAtEULA
                + '}';
    }
}