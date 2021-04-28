/**
 *  TdfBoolean.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

/**
 * Simple wrapper type for TDF boolean values.
 */
public class TdfBoolean implements Comparable<TdfBoolean> {
	
	/** The boolean value. */
	private Boolean mBooleanVal;
	
	/**
	 * Instantiates a new tdf boolean.
	 */
	public TdfBoolean() {
		mBooleanVal = new Boolean(false);
	}
	
	/**
	 * Instantiates a new tdf boolean.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfBoolean(TdfBoolean other) {
		mBooleanVal = other.get();
	}
	
	/**
	 * Instantiates a new tdf boolean.
	 * 
	 * @param booleanVal
	 *            the boolean val
	 */
	public TdfBoolean(Boolean booleanVal) {
		mBooleanVal = booleanVal;
	}
	
	/**
	 * Instantiates a new tdf boolean.
	 * 
	 * @param booleanVal
	 *            the boolean val
	 */
	public TdfBoolean(boolean booleanVal) {
		mBooleanVal = new Boolean(booleanVal);
	}
	
	/**
	 * Gets the value.
	 * 
	 * @return the boolean
	 */
	public Boolean get() {
		return mBooleanVal;
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mBooleanVal.hashCode();
	}
	
	/**
	 * Sets the value
	 * 
	 * @param booleanVal
	 *            the boolean val
	 */
	public void set(Boolean booleanVal) {
		mBooleanVal = booleanVal;
	}
	
	/**
	 * Returns boolean value.
	 * 
	 * @return wrapped boolean value
	 */
	public boolean booleanValue() { 
		return mBooleanVal.booleanValue();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mBooleanVal.toString();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfBoolean o) {
    	return mBooleanVal.compareTo(o.get());
    }
}
