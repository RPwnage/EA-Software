package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.scripts.tools.OAGDPPurchaseAllGames;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * To purchase all games through their GDP page as a subscriber
 *
 * @author svaghayenegar
 */
public class OAGDPPurchaseAllGamesSubscriber extends OAGDPPurchaseAllGames {

    @Test(groups = {"gdp_tools"})
    public void testGDPPurchaseAllGamesSubscriber(ITestContext context) throws Exception {
        testGDPPurchaseAllGames(context, true);
    }
}