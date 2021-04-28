package com.esn.sonar.allinone;

import com.esn.sonar.core.util.ByteString;
import com.esn.sonar.core.util.Utils;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ByteStringTest
{
    @Test
    public void integerToString()
    {
        assertEquals("31337", ByteString.fromLong(31337).toString());
        assertEquals("211", ByteString.fromLong(211).toString());
        assertEquals("13", ByteString.fromLong(13).toString());
        assertEquals("0", ByteString.fromLong(0).toString());
        assertEquals("2147483647", ByteString.fromLong(2147483647).toString());

        assertEquals("-31337", ByteString.fromLong(-31337).toString());
        assertEquals("-211", ByteString.fromLong(-211).toString());
        assertEquals("-13", ByteString.fromLong(-13).toString());
        assertEquals("0", ByteString.fromLong(-0).toString());
        assertEquals("-2147483647", ByteString.fromLong(-2147483647).toString());

    }

    @Test
    public void stringToInteger()
    {
        assertEquals(31337, new ByteString("31337").toLong());
        assertEquals(211, new ByteString("211").toLong());
        assertEquals(0, new ByteString("0").toLong());
        assertEquals(2147483647, new ByteString("2147483647").toLong());
        assertEquals(-31337, new ByteString("-31337").toLong());
        assertEquals(-211, new ByteString("-211").toLong());
        assertEquals(0, new ByteString("-0").toLong());
        assertEquals(-2147483647, new ByteString("-2147483647").toLong());
    }

    @Test
    public void tokenize()
    {
        String s1 = new String("jonas\t31337\t\t");

        String[] stokens = Utils.tokenize(s1, '\t');

        ByteString t1 = new ByteString("jonas\t31337\t\t");

        ByteString[] btokens = ByteString.tokenize(t1, ByteString.tabCharacter);
        assertEquals(stokens.length, btokens.length);

    }


}
