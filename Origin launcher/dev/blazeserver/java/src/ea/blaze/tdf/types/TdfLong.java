/**
 *  TdfLong.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

/**
 * Simple wrapper class for TDF long members.
 */
public class TdfLong implements Comparable<TdfLong> {
	
	/** The Constant MAX_VALUE. */
	public static final TdfLong MAX_VALUE = new TdfLong(Long.MAX_VALUE);
	
	/** The Constant MIN_VALUE. */
	public static final TdfLong MIN_VALUE = new TdfLong(Long.MIN_VALUE);
	
	/** The long val. */
	private Long mLongVal;
	
	/**
	 * Instantiates a new tdf long.
	 */
	public TdfLong() {
		mLongVal = new Long(0);
	}
	
	/**
	 * Instantiates a new tdf long.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfLong(TdfLong other) {
		mLongVal = other.get();
	}
	
	/**
	 * Instantiates a new tdf long.
	 * 
	 * @param longVal
	 *            the long val
	 */
	public TdfLong(Long longVal) {
		mLongVal = longVal;
	}
	
	/**
	 * Instantiates a new tdf long.
	 * 
	 * @param longVal
	 *            the long val
	 */
	public TdfLong(long longVal) {
		mLongVal = new Long(longVal);
	}
	
	/**
	 * Gets the value
	 * 
	 * @return the long
	 */
	public Long get() {
		return mLongVal;
	}
	
	/**
	 * Sets the value
	 * 
	 * @param longVal
	 *            the long val
	 */
	public void set(Long longVal) {
		mLongVal = longVal;
	}
	
	/**
	 * Long value.
	 * 
	 * @return the long value
	 */
	public long longValue() { 
		return mLongVal.longValue();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mLongVal.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mLongVal.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfLong o) {
	    return mLongVal.compareTo(o.get());
    }
}
