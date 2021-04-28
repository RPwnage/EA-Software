package com.esn.sonar.core.util;

import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;

/**
 * User: ronnie
 * Date: 2011-06-21
 * Time: 10:31
 */
public class ListSplitterTest
{

    private List<Integer> createList(int size)
    {
        return Utils.createSequence(0, size);
    }

    @Test
    public void testSplitEven()
    {
        List<Integer> expected = new ArrayList<Integer>(1000);
        List<Integer> list = createList(1000);
        int numSlices = 4;
        for (int i = 0; i < numSlices; i++)
        {
            expected.addAll(Utils.extractRange(list, numSlices, i));
        }
        assertEquals(expected, list);
    }

    @Test
    public void testSplitUneven()
    {
        List<Integer> expected = new ArrayList<Integer>(1000);
        List<Integer> list = createList(1000);
        int numSlices = 7;
        for (int i = 0; i < numSlices; i++)
        {
            expected.addAll(Utils.extractRange(list, numSlices, i));
        }
        assertEquals(expected, list);
    }

    @Test
    public void testSplitOne()
    {
        List<Integer> expected = new ArrayList<Integer>(1000);
        List<Integer> list = createList(1000);
        int numSlices = 1;
        for (int i = 0; i < numSlices; i++)
        {
            expected.addAll(Utils.extractRange(list, numSlices, i));
        }
        assertEquals(expected, list);
    }
}
