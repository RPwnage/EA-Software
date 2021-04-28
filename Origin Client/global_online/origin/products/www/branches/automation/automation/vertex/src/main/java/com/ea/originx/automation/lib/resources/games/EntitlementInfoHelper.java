package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.util.ArrayList;
import java.util.List;

public class EntitlementInfoHelper {

    private EntitlementInfoHelper() {
    }

    /**
     * Get {@link EntitlementInfo} instance for the specified
     * {@link EntitlementId}.
     *
     * @param entitlementId the {@link EntitlementId} that you want
     * EntitlementInfo for
     * @return {@link EntitlementInfo} that contains information for the
     * specified EntitlementId
     */
    public static EntitlementInfo getEntitlementInfoForId(EntitlementId entitlementId) {
        return EntitlementInfo.configure()
                .parentName(entitlementId.getParentName())
                .name(entitlementId.getName())
                .offerId(entitlementId.getOfferId())
                .ocdPath(entitlementId.getOcdPath())
                .productCode(entitlementId.getProductCode())
                .directoryName(entitlementId.getDirectoryName())
                .activationDirectoryName(entitlementId.getActivationDirectoryName())
                .processName(entitlementId.getProcessName())
                .downloadSize(entitlementId.getDownloadSize())
                .installSize(entitlementId.getInstallSize())
                .partialPdpUrl(entitlementId.getPartialPdpUrl())
                .masterTitleId(entitlementId.getMasterTitleId())
                .contentId(entitlementId.getContentId())
                .multiplayerId(entitlementId.getMultiplayerId())
                .localSavePath(entitlementId.getLocalSavePath())
                .windowsGUIDCode(entitlementId.getWindowsGUIDCode())
                .build();
    }

    /**
     * Returns the list of offerids for a give set of entitlements
     *
     * @param entitlementInfos entitlements
     * @return list of offerids of entitlements
     */
    public static List<String> getEntitlementOfferIds(EntitlementInfo... entitlementInfos) {
        List<String> offerids = new ArrayList<>();
        for (EntitlementInfo entitlement : entitlementInfos) {
            offerids.add(entitlement.getOfferId());
        }
        return offerids;
    }

    /**
     * Get the process name for a give entitlement name
     *
     * @param entitlementName the entitlement name of the game to get the
     * process name
     * @return process name if the entitlement is found or else null
     */
    public static String getProcessName(String entitlementName) {
        if (entitlementName != null) {
            for (EntitlementId entitlementId : EntitlementId.values()) {
                if (entitlementName.equalsIgnoreCase(entitlementId.getName())) {
                    return entitlementId.getProcessName();
                }
            }
        }
        return null;
    }
}
