/**
 *  Base64Encoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.protocol;

import java.io.ByteArrayOutputStream;

/**
 * The Base64Encoder handles encoding and decoding of Base64 strings.
 */
public class Base64Encoder
{
	
	/** The Constant DEFAULT_FILLCHAR. */
	private static final char DEFAULT_FILLCHAR = '=';

	/** The Constant DEFAULT_CHARMAP. */
	private static final String DEFAULT_CHARMAP =
		// 00000000001111111111222222
		// 01234567890123456789012345
		  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

		// 22223333333333444444444455
		// 67890123456789012345678901
		+ "abcdefghijklmnopqrstuvwxyz"

		// 555555556666
		// 234567890123
		+ "0123456789+/";

	/** The fill char. */
	private final char mFillChar;
	
	/** The char map. */
	private final String mCharMap;

	/**
	 * Instantiates a new base64 encoder.
	 */
	public Base64Encoder() {
		this(DEFAULT_FILLCHAR, DEFAULT_CHARMAP);
	}

	/**
	 * Instantiates a new base64 encoder.
	 * 
	 * @param fillChar
	 *            the fill char
	 * @param charMap
	 *            the char map
	 */
	public Base64Encoder(char fillChar, String charMap) {
		if ((charMap == null) || (charMap.length() != 64))
			throw new IllegalArgumentException("Character map must be exactly 64 characters");
		if (charMap.indexOf(fillChar) >= 0)
			throw new IllegalArgumentException("Character map cannot contain the fill character");
		mFillChar = fillChar;
		mCharMap = charMap;
	}

	/**
	 * Encodes the data into a Base64 string.
	 * 
	 * @param data
	 *            the data to encode
	 * @return the base64 encoded string.
	 */
	public String encode(byte[] data)
	{
		int c;
		int len = data.length;
		StringBuffer ret = new StringBuffer(((len / 3) + 1) * 4);
		for (int i = 0; i < len; ++i) {
			c = (data[i] >> 2) & 0x3f;
			ret.append(mCharMap.charAt(c));
			c = (data[i] << 4) & 0x3f;
			if (++i < len)
				c |= (data[i] >> 4) & 0x0f;

			ret.append(mCharMap.charAt(c));
			if (i < len) {
				c = (data[i] << 2) & 0x3f;
				if (++i < len)
					c |= (data[i] >> 6) & 0x03;

				ret.append(mCharMap.charAt(c));
			}
			else {
				++i;
				ret.append(mFillChar);
			}

			if (i < len) {
				c = data[i] & 0x3f;
				ret.append(mCharMap.charAt(c));
			}
			else {
				ret.append(mFillChar);
			}
		}

		return ret.toString();
	}

	/**
	 * Decode bytes from a base64 encoded string into a byte array.
	 * 
	 * @param data
	 *            the string data to decode.
	 * @return the decoded bytes
	 */
	public byte[] decode(byte[] data)
	{
		int op;
		int op1;
		int len = data.length;
		ByteArrayOutputStream baos = new ByteArrayOutputStream();

		for (int i = 0; i < len; ++i) {
			op = mCharMap.indexOf(data[i]);
			++i;
			op1 = mCharMap.indexOf(data[i]);
			op = ((op << 2) | ((op1 >> 4) & 0x3));
			baos.write((byte) op);
			if (++i < len) {
				op = data[i];
				if (mFillChar == op)
					break;

				op = mCharMap.indexOf((char) op);
				op1 = ((op1 << 4) & 0xf0) | ((op >> 2) & 0xf);
				baos.write((byte) op1);
			}

			if (++i < len) {
				op1 = data[i];
				if (mFillChar == op1)
					break;

				op1 = mCharMap.indexOf((char) op1);
				op = ((op << 6) & 0xc0) | op1;
				baos.write((byte) op);
			}
		}

		return baos.toByteArray();
	}
}
