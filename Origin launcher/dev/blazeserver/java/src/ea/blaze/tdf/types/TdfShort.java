/**
 *  TdfShort.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

/**
 * Simple wrapper class for TDF short values.
 */
public class TdfShort implements Comparable<TdfShort> {
	
	/** The Constant MAX_VALUE. */
	public static final TdfShort MAX_VALUE = new TdfShort(Short.MAX_VALUE);
	
	/** The Constant MIN_VALUE. */
	public static final TdfShort MIN_VALUE = new TdfShort(Short.MIN_VALUE);
	
	/** The short val. */
	private Short mShortVal;
	
	/**
	 * Instantiates a new tdf short.
	 */
	public TdfShort() {
		mShortVal = new Short((short)0);
	}
	
	/**
	 * Instantiates a new tdf short.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfShort(TdfShort other) {
		mShortVal = other.get();
	}
	
	/**
	 * Instantiates a new tdf short.
	 * 
	 * @param shortVal
	 *            the short val
	 */
	public TdfShort(Short shortVal) {
		mShortVal = shortVal;
	}
	
	/**
	 * Instantiates a new tdf short.
	 * 
	 * @param shortVal
	 *            the short val
	 */
	public TdfShort(short shortVal) {
		mShortVal = new Short(shortVal);
	}
	
	/**
	 * Gets the value
	 * 
	 * @return the short
	 */
	public Short get() {
		return mShortVal;
	}
	
	/**
	 * Sets the value
	 * 
	 * @param shortVal
	 *            the short val
	 */
	public void set(Short shortVal) {
		mShortVal = shortVal;
	}
	
	/**
	 * Short value.
	 * 
	 * @return the short value
	 */
	public short shortValue() { 
		return mShortVal.shortValue();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mShortVal.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mShortVal.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfShort o) {
	    return mShortVal.compareTo(o.get());
    }
}
