package com.ea.originx.automation.lib.resources.games;

import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.resources.EACore;
import com.ea.vx.originclient.net.helpers.RestfulHelper;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.net.URISyntaxException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * A class to manage the Small DiP entitlement
 *
 * @author rmcneil
 */
public final class OADipSmallGame extends EntitlementInfo {

    private static final String DIP_SMALL_SHORTCUT_NAME = EntitlementInfoConstants.DIP_SMALL_NAME + " en_GB";
    private static final String DIP_SMALL_OVERRIDE = EntitlementInfoConstants.DIP_SMALL_OFFER_ID;
    public static final String DIP_SMALL_AUTO_DOWNLOAD_PATH = EACore.EACORE_DOWNLOAD_OVERRIDE + DIP_SMALL_OVERRIDE;

    // DLC
    public static final String PDLC_A_NAME = "Client Automation DiP PDLC_A";
    public static final String PDLC_A_OFFER_ID = "Origin.OFR.50.0000401";

    // Cloud URLs
    private static final String CLOUD_CLIENT_AUTOMATION_URL = "https://s3.amazonaws.com/client-automation/entitlements/";
    public static final String DIP_SMALL_DOWNLOAD_VALUE_UPDATE_TINY = CLOUD_CLIENT_AUTOMATION_URL + "automation_dip_small_v2.zip";
    private static final String DIP_SMALL_DOWNLOAD_VALUE_UPDATE_MEDIUM = CLOUD_CLIENT_AUTOMATION_URL + "automation_dip_medium.zip";
    public static final String DIP_SMALL_DOWNLOAD_VALUE_UPDATE_LARGE = CLOUD_CLIENT_AUTOMATION_URL + "automation_dip_large.zip";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor. Make private as this class should never be instantiated
     */
    public OADipSmallGame() {
        super(configure()
                .name(EntitlementInfoConstants.DIP_SMALL_NAME)
                .offerId(EntitlementInfoConstants.DIP_SMALL_OFFER_ID)
                .productCode(OSInfo.isProduction() ? EntitlementInfoConstants.DIP_SMALL_PRODUCT_CODE_PRODUCTION : EntitlementInfoConstants.DIP_SMALL_PRODUCT_CODE)
                .directoryName(EntitlementInfoConstants.DIP_SMALL_DIRECTORY_NAME)
                .processName(EntitlementInfoConstants.FAKE_ENTITLEMENT_PROCESS_NAME)
                .downloadSize(EntitlementInfoConstants.DIP_SMALL_DOWNLOAD_SIZE)
                .installSize(EntitlementInfoConstants.DIP_SMALL_INSTALL_SIZE)
                .windowsGUIDCode(EntitlementInfoConstants.DIP_SMALL_WINDOWS_GUID_CODE)
        );
    }

    /**
     * Get entitlement shortcut name - for Windows start menu or desktop
     * shortcuts
     *
     * @return entitlement shortcut name, or empty string if not defined
     */
    public String getShortcutName() {
        return DIP_SMALL_SHORTCUT_NAME;
    }

     /**
     * Wait for the game to launch.
     *
     * @param client {@link OriginClient} instance
     * @return true if DiPSmall is open, else false
     */
    public boolean waitForGameLaunch(OriginClient client) {
        return Waits.pollingWait(() -> this.isWindowOpen(client), 60000, 1000, 0);
    }

    /**
     * Hack to check if the window is open or closed
     *
     * @param client {@link OriginClient} instance
     * @return true if the window is open, false if not or on error
     */
    private boolean isWindowOpen(OriginClient client) {
        /*
         * Since the process appears a few seconds before the window does,
         * waiting for the process doesn't work. However, the process uses 16K
         * memory until the window opens in which the memory usage jumps to 48k.
         * So we wait for that instead. If there are multiple processes, getProcessMemory
         * sums the memory, we also need to divide by the total number of processes.
         */

        int numProcesses;
        try {
            numProcesses = Math.max(ProcessUtil.getNumberOfProcessInstances(client, getProcessName()), 1);
        } catch (IOException | InterruptedException e) {
            return false;
        }
        return ProcessUtil.getProcessMemory(client, getProcessName()) / numProcesses > 20000;
    }

    /**
     * Returns true if DiP Small is launched
     *
     * @param client {@link OriginClient} instance
     * @return true if the game is launched and the window is opened, else false
     * @throws java.io.IOException if an I/O exception occurs.
     * @throws java.lang.InterruptedException if thread is interrupted while
     * waiting/sleeping
     */
    @Override
    public boolean isLaunched(OriginClient client) throws IOException, InterruptedException {
        return isWindowOpen(client) && super.isLaunched(client);
    }

    /**
     * To change the Entitlement to Medium version
     *
     * @param client {@link OriginClient} instance
     * @return true if set successfully, false otherwise.
     */
    public static boolean changeToMediumEntitlementDipSmallOverride(OriginClient client) {
        return client.getEACore()
                .setEACoreValue(EACore.EACORE_CONNECTION_SECTION,
                        DIP_SMALL_AUTO_DOWNLOAD_PATH,
                        DIP_SMALL_DOWNLOAD_VALUE_UPDATE_MEDIUM);
    }

    /**
     * To change the Entitlement's downloaded path to invalid path
     *
     * @param client {@link OriginClient} instance
     * @return true if set successfully, false otherwise.
     */
    public static boolean setInvalidPathOverride(OriginClient client) {
        return client.getEACore().setEACoreValue(EACore.EACORE_CONNECTION_SECTION,
                DIP_SMALL_AUTO_DOWNLOAD_PATH,
                OriginClientData.INVALID_PATH);
    }
}
