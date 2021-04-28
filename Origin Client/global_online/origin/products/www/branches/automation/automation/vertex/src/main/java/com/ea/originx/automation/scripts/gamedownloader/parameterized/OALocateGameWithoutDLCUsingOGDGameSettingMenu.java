package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OALocateGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Locate game without DLC using OGD game setting menu
 *
 * @author sunlee
 */
public class OALocateGameWithoutDLCUsingOGDGameSettingMenu extends OALocateGame {

    @TestRail(caseId = 4416567)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testLocateGameWithoutDLCUsingOGDGameSettingMenu(ITestContext context) throws Exception {
        testLocateGame(context, false, METHOD_TO_LOCATE_GAME.USE_OGD_GAME_SETTINGS, this.getClass().getName());
    }
}
