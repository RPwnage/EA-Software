/**
 *  Heat2Util.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.protocol;

/**
 * This class contains constants used for both Heat2Encoder and Heat2Decoder.
 */
public class Heat2Util {
	
	
	/** The Constant ELEMENT_TYPE_MAX. */
	public static final int ELEMENT_TYPE_MAX = 0x1f;
	
	/** The Constant ID_TERM. */
	public static final byte ID_TERM = 0x00;
	
	/** The Constant VARSIZE_MORE. */
	public static final int VARSIZE_MORE = 0x80;
	
	/** The Constant VARSIZE_NEGATIVE. */
	public static final int VARSIZE_NEGATIVE = 0x40;
	
	/** The Constant VARSIZE_VALUE_MASK. */
	public static final int VARSIZE_VALUE_MASK = 0x3f;
	
	/** The Constant VARSIZE_MAX_ENCODE_SIZE. */
	public static final int VARSIZE_MAX_ENCODE_SIZE = 10;
	
	/** The Constant FLOAT_SIZE. */
	public static final int FLOAT_SIZE = 4;
	
	/** The Constant HEADER_TYPE_OFFSET. */
	public static final int HEADER_TYPE_OFFSET = 3;
	
	/** The Constant HEADER_SIZE. */
	public static final int HEADER_SIZE = 4;
	
	/** The Constant MAX_TAG_LENGTH. */
	public static final int MAX_TAG_LENGTH = 4;
	
	/** The Constant TAG_CHAR_MIN. */
	public static final int TAG_CHAR_MIN = 32;
	
	/** The Constant TAG_CHAR_MAX. */
	public static final int TAG_CHAR_MAX = 95;

	/** The Constant TAG_UNSPECIFIED. */
	public static final int TAG_UNSPECIFIED = 0;
	
	
	/**
	 * Promote unsigned byte to signed short.
	 * 
	 * @param byteVal
	 *            the byte val containing unsigned byte to be promoted.
	 * @return the promoted short value
	 */
	public static short promoteUnsignedByte(byte byteVal) {
		return (short)(byteVal & 0xff);
	}

	/**
	 * Promote unsigned short to signed integer
	 * 
	 * @param shortVal
	 *            the short val containing unsigned short to be promoted.
	 * @return the promoted int value
	 */
	public static int promoteUnsignedShort(short shortVal) {
		return (int)(shortVal & 0xffff);
	}
	
	/**
	 * Promote unsigned integer to signed long
	 * 
	 * @param intVal
	 *            the int val containing unsigned int to be promoted.
	 * @return the promoted long value.
	 */
	public static long promoteUnsignedInteger(int intVal) {
		return (long) (intVal & 0xffffffff);
	}
}
