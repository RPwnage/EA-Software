package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * Class representing StarWarsBattlefront
 *
 * @author palui
 */
public final class StarWarsBattlefront extends EntitlementInfo {

    public static final String STAR_WAR_BATTLEFRONT_DEATH_STAR_NAME = "STAR WARS™ Battlefront™ Death Star";
    public static final String STAR_WAR_BATTLEFRONT_DEATH_STAR_OFFER_ID = "Origin.OFR.50.0001169";
    public static final String STAR_WAR_BATTLEFRONT_DEATH_STAR_PARTIAL_PDP_URL = "/store/star-wars/star-wars-battlefront/expansion/star-wars-battlefront-death-star?";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor. Make private as this class should never be instantiated
     */
    public StarWarsBattlefront() {
        super(configure()
                .name(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_NAME)
                .offerId(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_OFFER_ID)
                .partialPdpUrl(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_PARTIAL_PDP_URL)
        );
    }
}
