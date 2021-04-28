package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import java.lang.invoke.MethodHandles;

/**
 * A class to manage FIFA 18 DLCs.
 *
 * @author caleung
 */
public final class Fifa18 extends EntitlementInfo {

    public static final String FIFA18_100POINTS_PACK_NAME = "100 FIFA 18 Points Pack";
    public static final String FIFA18_100POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002055";
    public static final String FIFA18_250POINTS_PACK_NAME = "250 FIFA 18 Points Pack";
    public static final String FIFA18_250POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002056";
    public static final String FIFA18_500POINTS_PACK_NAME = "500 FIFA 18 Points Pack";
    public static final String FIFA18_500POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002057";
    public static final String FIFA18_750POINTS_PACK_NAME = "750 FIFA 18 Points Pack";
    public static final String FIFA18_750POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002058";
    public static final String FIFA18_1050POINTS_PACK_NAME = "1,050 FIFA 18 Points Pack";
    public static final String FIFA18_1050POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002059";
    public static final String FIFA18_1600POINTS_PACK_NAME = "1,600 FIFA 18 Points Pack";
    public static final String FIFA18_1600POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002060";
    public static final String FIFA18_2200POINTS_PACK_NAME = "2,200 FIFA 18 Points Pack";
    public static final String FIFA18_2200POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002061";
    public static final String FIFA18_4600POINTS_PACK_NAME = "4,600 FIFA 18 Points Pack";
    public static final String FIFA18_4600POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002062";
    public static final String FIFA18_12000POINTS_PACK_NAME = "12,000 FIFA 18 Points Pack";
    public static final String FIFA18_12000POINTS_PACK_OFFER_ID = "Origin.OFR.50.0002063";

    public static final String FIFA18_CURRENCY_PACK_OFFER_IDS[] = {
            FIFA18_100POINTS_PACK_OFFER_ID,
            FIFA18_250POINTS_PACK_OFFER_ID,
            FIFA18_500POINTS_PACK_OFFER_ID,
            FIFA18_750POINTS_PACK_OFFER_ID,
            FIFA18_1050POINTS_PACK_OFFER_ID,
            FIFA18_1600POINTS_PACK_OFFER_ID,
            FIFA18_2200POINTS_PACK_OFFER_ID,
            FIFA18_4600POINTS_PACK_OFFER_ID,
            FIFA18_12000POINTS_PACK_OFFER_ID
    };

    public static final String FIFA18_CURRENCY_PACK_NAMES[] = {
            FIFA18_100POINTS_PACK_NAME,
            FIFA18_250POINTS_PACK_NAME,
            FIFA18_500POINTS_PACK_NAME,
            FIFA18_750POINTS_PACK_NAME,
            FIFA18_1050POINTS_PACK_NAME,
            FIFA18_1600POINTS_PACK_NAME,
            FIFA18_2200POINTS_PACK_NAME,
            FIFA18_4600POINTS_PACK_NAME,
            FIFA18_12000POINTS_PACK_NAME
    };

    public static final String FIFA18_100POINTS_PACK_PARTIAL_PDP_URL = "/store/fifa/fifa-18/currency?";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor - Make private as this class should never be instantiated
     */
    public Fifa18() {
        super(configure()
                .name(EntitlementInfoConstants.FIFA_18_ICON_NAME)
                .offerId(EntitlementInfoConstants.FIFA_18_ICON_NAME)
                .partialPdpUrl(EntitlementInfoConstants.FIFA_18_ICON_NAME)
        );
    }
}