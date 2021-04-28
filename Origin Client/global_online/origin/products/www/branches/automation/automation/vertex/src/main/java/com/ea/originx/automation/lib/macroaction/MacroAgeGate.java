/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ea.originx.automation.lib.macroaction;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.Select;

/**
 * Macro class containing static methods to be used with the SPA Age Gate.
 * 
 * @author sbentley
 */
public final class MacroAgeGate {
    
    private static final String VIDEO_AGEGATE_VALUE_UNDER_TEN = "string:0";
    private static final String VIDEO_AGEGATE_VALUE_OVER_TEN = "number:";
    
    private MacroAgeGate() {
    }
    
    /**
     * Enters the given birth date into an SPA age gate.
     * 
     * @param birthday Birthday formatted from MiscUtilities
     * @param ageGateSelects The WebElement of the selects used to enter mm/dd/yy
     * @param okayButton The button to confirm the entered birth date
     * @throws ParseException If there is an error parsing
     */
    public static void enterVideoAgeGateBirthday(String birthday, List<WebElement> ageGateSelects, WebElement okayButton) throws ParseException {
        
        //There should be 3 for month/day/year
        if (ageGateSelects.size() != 3) {
            throw new RuntimeException("Incorrect number of selects for Month/Day/Year");
        }
        
        //Each select is the first child of the ageGateSelects
        WebElement monthElement = ageGateSelects.get(0).findElements(By.xpath(".//*")).get(0);
        WebElement dayElement = ageGateSelects.get(1).findElements(By.xpath(".//*")).get(0);
        WebElement yearElement = ageGateSelects.get(2).findElements(By.xpath(".//*")).get(0);

        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy/MMMM/dd");
        Calendar cal = Calendar.getInstance();
        cal.setTime(dateFormat.parse(birthday));

        int calendarDay = cal.get(Calendar.DAY_OF_MONTH);
        int calendarMonth = cal.get(Calendar.MONTH) + 1; //Month is zero based
        int calendarYear = cal.get(Calendar.YEAR);

        String dayValue = calendarDay < 10 ? VIDEO_AGEGATE_VALUE_UNDER_TEN + calendarDay : VIDEO_AGEGATE_VALUE_OVER_TEN + calendarDay;
        String monthValue = calendarMonth < 10 ? VIDEO_AGEGATE_VALUE_UNDER_TEN + calendarMonth : VIDEO_AGEGATE_VALUE_OVER_TEN + calendarMonth;
        String yearValue = VIDEO_AGEGATE_VALUE_OVER_TEN + calendarYear;

        new Select(monthElement).selectByValue(monthValue);
        new Select(dayElement).selectByValue(dayValue);
        new Select(yearElement).selectByValue(yearValue);

        okayButton.click();
    }
}
