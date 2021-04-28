package com.ea.originx.automation.lib.macroaction;

import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.FirewallUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to Origin Client Network Offline
 *
 * @author palui
 */
public final class MacroNetworkOffline {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     */
    private MacroNetworkOffline() {
    }

    /**
     * Simulate Origin client Network Offline by blocking Origin client with the
     * firewall and then temporarily disabling Windows Network services to allow
     * Origin client to detect Network Offline. Once in Network Offline, Origin
     * client should stay there until firewall unblocks.
     *
     * @param driver Selenium WebDriver. If null (e.g. call before login), skip
     * offline mode indicator check
     * @return true if Origin client can be entered into Network Offline, false
     * otherwise
     */
    public static boolean enterOriginNetworkOffline(WebDriver driver) {
        final OriginClient client = null == driver ? null : OriginClient.getInstance(driver);
        return FirewallUtil.blockOrigin(client)
                && (driver == null || Waits.pollingWait(new MainMenu(driver)::verifyOfflineModeButtonText, 60000, 500, 0));
    }

    /**
     * Exit Origin client from simulated Network Offline by re-enabling Windows
     * Network Services and unblocking Origin client at the firewall.
     *
     * @param driver Selenium WebDriver. If null (e.g. call before login), skip
     * offline mode indicator absence check
     * @return true if exited Origin from Network Offline back to Online, false
     * otherwise
     */
    public static boolean exitOriginNetworkOffline(WebDriver driver) {
        final OriginClient client = null == driver ? null : OriginClient.getInstance(driver);
        return FirewallUtil.allowOrigin(client)
                && (driver == null || Waits.pollingWait(() -> !new MainMenu(driver).verifyOfflineModeButtonText(), 60000, 500, 0));
    }
}