package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;

/**
 * We use Notepad as a "game" for testing non origin games
 *
 * @author jmittertreiner
 */
public final class Notepad extends EntitlementInfo {

    public Notepad() {
        super(configure()
                .name("notepad")
                .directoryName("C:\\windows\\system32")
                .offerId("NOG_783301695")
                .processName("notepad.exe")
        );
    }

    /**
     * @param client {@link OriginClient} instance
     * @return true if launched, false otherwise
     * @throws java.io.IOException if an I/O exception occurs.
     * @throws java.lang.InterruptedException if thread is interrupted while
     * waiting/sleeping
     */
    @Override
    public boolean isLaunched(OriginClient client) throws IOException, InterruptedException {
        // Don't check RAM on notepad
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
}
