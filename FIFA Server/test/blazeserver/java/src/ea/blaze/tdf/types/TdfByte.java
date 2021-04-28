/**
 *  TdfByte.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

/**
 * Simple wrapper type for TDF byte values.
 */
public class TdfByte implements Comparable<TdfByte> {
	
	/** The Constant MAX_VALUE. */
	public static final TdfByte MAX_VALUE = new TdfByte(Byte.MAX_VALUE);
	
	/** The Constant MIN_VALUE. */
	public static final TdfByte MIN_VALUE = new TdfByte(Byte.MIN_VALUE);
	
	/** The byte val. */
	private Byte mByteVal;
	
	/**
	 * Instantiates a new tdf byte.
	 */
	public TdfByte() {
		mByteVal = new Byte((byte)0);
	}
	
	/**
	 * Instantiates a new tdf byte.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfByte(TdfByte other) {
		mByteVal = other.get();
	}
	
	/**
	 * Instantiates a new tdf byte.
	 * 
	 * @param byteVal
	 *            the byte val
	 */
	public TdfByte(Byte byteVal) {
		mByteVal = byteVal;
	}
	
	/**
	 * Instantiates a new tdf byte.
	 * 
	 * @param byteVal
	 *            the byte val
	 */
	public TdfByte(byte byteVal) {
		mByteVal = new Byte(byteVal);
	}
	
	/**
	 * Gets the value
	 * 
	 * @return the byte
	 */
	public Byte get() {
		return mByteVal;
	}
	
	/**
	 * Sets the value
	 * 
	 * @param byteVal
	 *            the byte val
	 */
	public void set(Byte byteVal) {
		mByteVal = byteVal;
	}
	
	/**
	 * Returns byte
	 * 
	 * @return the byte
	 */
	public byte byteValue() { 
		return mByteVal.byteValue();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mByteVal.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mByteVal.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfByte o) {
	    return mByteVal.compareTo(o.get());
    }
}
