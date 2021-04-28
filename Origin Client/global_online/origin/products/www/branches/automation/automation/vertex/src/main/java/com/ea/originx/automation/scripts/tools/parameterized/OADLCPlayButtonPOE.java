package com.ea.originx.automation.scripts.tools.parameterized;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.scripts.tools.OADLCPlayButton;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * @author sbentley
 */

/**
 * Parameterized test script for OADLCPlayButton<br>
 * parameters:<br>
 * gameFamilyType: Pillars of Eternity<br>
 *
 * @author sbentley
 */
public class OADLCPlayButtonPOE extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonPOE(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.PILLARS_OF_ETERNITY);
    }
}
