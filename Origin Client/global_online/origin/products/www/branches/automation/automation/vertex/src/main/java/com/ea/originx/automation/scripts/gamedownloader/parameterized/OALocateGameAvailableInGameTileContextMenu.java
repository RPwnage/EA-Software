package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OALocateGameAvailable;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Locate Game' is available in game tile context menu if the game is downloadable
 *
 * @author sunlee
 */
public class OALocateGameAvailableInGameTileContextMenu extends OALocateGameAvailable {

    @TestRail(caseId = 4383140)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testLocateGameAvailableInGameTileContextMenu(ITestContext context) throws Exception {
        testLocateGameAvailable(context, METHOD_TO_LOCATE_GAME.USE_GAME_TILE_CONTEXT_MENU, this.getClass().getName());
    }
}

