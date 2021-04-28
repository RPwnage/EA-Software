/**
 *  TdfInteger.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

/**
 * Simple wrapper class for TDF integer values.
 */
public class TdfInteger implements Comparable<TdfInteger> {
	
	/** The Constant MAX_VALUE. */
	public static final TdfInteger MAX_VALUE = new TdfInteger(Integer.MAX_VALUE);
	
	/** The Constant MIN_VALUE. */
	public static final TdfInteger MIN_VALUE = new TdfInteger(Integer.MIN_VALUE);
	
	/** The int val. */
	private Integer mIntVal;
	
	/**
	 * Instantiates a new tdf integer.
	 */
	public TdfInteger() {
		mIntVal = new Integer(0);
	}
	
	/**
	 * Instantiates a new tdf integer.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfInteger(TdfInteger other) {
		mIntVal = other.get();
	}
	
	/**
	 * Instantiates a new tdf integer.
	 * 
	 * @param intVal
	 *            the int val
	 */
	public TdfInteger(Integer intVal) {
		mIntVal = intVal;
	}
	
	/**
	 * Instantiates a new tdf integer.
	 * 
	 * @param intVal
	 *            the int val
	 */
	public TdfInteger(int intVal) {
		mIntVal = new Integer(intVal);
	}
	
	/**
	 * Gets the value
	 * 
	 * @return the integer
	 */
	public Integer get() {
		return mIntVal;
	}
	
	/**
	 * Sets the value
	 * 
	 * @param intVal
	 *            the int val
	 */
	public void set(Integer intVal) {
		mIntVal = intVal;
	}
	
	/**
	 * Int value.
	 * 
	 * @return the int value
	 */
	public int intValue() { 
		return mIntVal.intValue();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mIntVal.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mIntVal.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfInteger o) {
	    return mIntVal.compareTo(o.get());
    }
}
