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
 * gameFamilyType: TW3GOTY<br>
 *
 * @author sbentley
 */
public class OADLCPlayButtonTW3GOTY extends OADLCPlayButton {

    @Test(groups = {"personalization", "client_only"})
    public void testDLCPlayButtonTW3GOTY(ITestContext context) throws Exception {
        testDLCPlayButton(context, GameFamilyType.THE_WITCHER_3_GOTY);
    }
}
