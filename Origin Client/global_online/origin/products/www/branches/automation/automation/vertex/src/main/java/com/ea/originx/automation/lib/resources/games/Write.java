package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;

/**
 * We use Wordpad as a "game" for testing non origin games
 *
 * @author jmittertreiner
 */
public final class Write extends EntitlementInfo {

    private final String _executableName;

    public Write() {
        super(configure()
                .name("write")
                .directoryName("C:\\windows\\system32")
                .offerId("NOG_743181061")
                .processName("wordpad.exe")
        );

        _executableName = "write.exe";
    }

    /**
     * Check if entitlement process has been launched
     *
     * @param client {@link OriginClient} instance
     * @return true if launched, false otherwise
     */
    @Override
    public boolean isLaunched(OriginClient client) {
        // Don't check RAM on write/wordpad since it's so small
        return Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, getProcessName()));
    }

    /**
     * @param client {@link OriginClient} instance
     * @return entitlement game directory full path
     */
    @Override
    public String getDirectoryFullPath(OriginClient client) {
        return getDirectoryName();
    }

    @Override
    public String getExecutablePath(OriginClient client) {
        return getDirectoryFullPath(client) + "\\" + _executableName;
    }

    /**
     * @return The executable name of the application, since it's different than
     * the process name
     */
    public String getExecutableName() {
        return _executableName;
    }
}
