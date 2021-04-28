package com.esn.sonar.core.util;

import org.jboss.netty.util.CharsetUtil;

import java.util.Deque;
import java.util.LinkedList;

public class ByteString
{
    public static final byte nilCharacter = "0".getBytes(CharsetUtil.US_ASCII)[0];
    public static final byte minusCharacter = "-".getBytes(CharsetUtil.US_ASCII)[0];
    public static final byte tabCharacter = "\t".getBytes(CharsetUtil.US_ASCII)[0];

    static final int longLength = "9223372036854775807".length();

    private final byte[] buffer;
    private int hash;
    private final int from;
    private final int to;

    public ByteString(String str)
    {
        buffer = str.getBytes(CharsetUtil.UTF_8);
        from = 0;
        to = buffer.length;
    }

    public ByteString(byte[] buffer)
    {
        this.buffer = buffer;
        this.from = 0;
        this.to = buffer.length;
    }

    public ByteString(byte[] buffer, int from, int to)
    {
        this.buffer = buffer;
        this.from = from;
        this.to = to;
    }

    byte charAt(int pos)
    {
        return buffer[from + pos];
    }

    @Override
    public int hashCode()
    {
        if (hash == 0)
        {
            for (int pos = from; pos < to; pos++)
            {
                hash = 31 * hash + buffer[pos];
            }
        }

        return hash;
    }

    static private boolean equalsRange(byte[] buffA, int fromA, int toA, byte[] buffB, int fromB, int toB)
    {
        int lenA = toA - fromA;
        int lenB = toB - fromB;

        if (lenA != lenB)
            return false;

        for (int index = 0; index < lenA; index++)
        {
            if (buffA[fromA] != buffB[fromB])
                return false;

            fromA++;
            fromB++;
        }

        return true;
    }

    @Override
    public boolean equals(Object obj)
    {
        if (!(obj instanceof ByteString)) return false;

        if (this == obj) return true;
        if (this.hashCode() != obj.hashCode()) return false;

        ByteString o = (ByteString) obj;
        return equalsRange(this.buffer, this.from, this.to, o.buffer, o.from, o.to);
    }

    @Override
    protected Object clone()
    {
        return new ByteString(buffer, from, to);
    }

    @Override
    public String toString()
    {
        return new String(buffer, from, to - from, CharsetUtil.UTF_8);
    }

    public int length()
    {
        return to - from;
    }

    public long toLong() throws NumberFormatException
    {
        int pos = this.to - 1;

        if (pos < 0)
            throw new NumberFormatException("Empty string");

        int end = 0;
        long retValue = 0;
        long multiplier = 1;

        while (pos >= end)
        {
            if (buffer[pos] == minusCharacter)
            {
                retValue -= (retValue * 2);
                break;
            }

            long value = (long) (buffer[pos] - nilCharacter);

            if (value < 0 || value > 9)
            {
                throw new NumberFormatException("Invalid character in string");
            }

            retValue += value * multiplier;

            pos--;
            multiplier *= 10;
        }

        return retValue;
    }

    static public ByteString fromLong(long value)
    {
        byte output[] = new byte[longLength];

        int pos = longLength - 1;

        boolean negative = false;

        if (value < 0)
        {
            value += value * -2;
            negative = true;
        }

        while (true)
        {
            byte chr = (byte) (value % 10);
            value /= 10;
            output[pos] = (byte) (nilCharacter + chr);
            pos--;

            if (value == 0)
                break;
        }

        if (negative)
        {
            output[pos] = minusCharacter;
            pos--;
        }

        return new ByteString(output, pos + 1, longLength);
    }


    public int findByte(int from, byte value)
    {
        for (int index = this.from + from; index < to; index++)
        {
            if (buffer[index] == value)
                return index - this.from;
        }

        return -1;
    }

    public ByteString substring(int start, int length)
    {
        return new ByteString(buffer, start, start + length);
    }

    public static ByteString[] tokenize(ByteString str, byte delimiter)
    {
        int length = str.length();

        Deque<ByteString> list = new LinkedList<ByteString>();

        int offset;
        int start = 0;

        while (true)
        {
            offset = str.findByte(start, delimiter);

            if (offset == -1)
            {
                list.add(str.substring(start, length - start));
                break;
            }

            list.add(str.substring(start, offset - start));

            offset++;
            start = offset;
        }

        return list.toArray(new ByteString[list.size()]);
    }

    public byte[] array()
    {
        return buffer;
    }
}
