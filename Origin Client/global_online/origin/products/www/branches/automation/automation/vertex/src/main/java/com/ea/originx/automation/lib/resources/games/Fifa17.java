package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.lang.invoke.MethodHandles;

/**
 * A class to manage FIFA 17 DLCs
 *
 * @author rocky
 */
public final class Fifa17 extends EntitlementInfo {

    public static final String FIFA17_100POINTS_PACK_NAME = "100 FIFA 17 Points Pack";
    public static final String FIFA17_100POINTS_PACK_OFFER_ID = "Origin.OFR.50.0001322";
    public static final String FIFA17_100POINTS_PACK_PARTIAL_PDP_URL = "/store/fifa/fifa-17/currency/100-fifa-17-points-pack?";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor. Make private as this class should never be instantiated
     */
    public Fifa17() {
        super(configure()
                .name(EntitlementInfoConstants.FIFA_17_DELUXE_NAME)
                .offerId(EntitlementInfoConstants.FIFA_17_DELUXE_OFFER_ID)
                .partialPdpUrl(EntitlementInfoConstants.FIFA_17_DELUXE_PARTIAL_PDP_URL)
        );
    }
}
