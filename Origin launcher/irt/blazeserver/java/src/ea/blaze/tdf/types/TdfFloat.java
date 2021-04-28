/**
 *  TdfFloat.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

/**
 * Simple wrapper class for TDF float values.
 */
public class TdfFloat implements Comparable<TdfFloat>{
	
	/** The Constant MAX_VALUE. */
	public static final TdfFloat MAX_VALUE = new TdfFloat(Float.MAX_VALUE);
	
	/** The Constant MIN_VALUE. */
	public static final TdfFloat MIN_VALUE = new TdfFloat(Float.MIN_VALUE);
	
	/** The float val. */
	private Float mFloatVal;
	
	/**
	 * Instantiates a new tdf float.
	 */
	public TdfFloat() {
		mFloatVal = new Float((float)0);
	}
	
	/**
	 * Instantiates a new tdf float.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfFloat(TdfFloat other) {
		mFloatVal = other.get();
	}
	
	/**
	 * Instantiates a new tdf float.
	 * 
	 * @param floatVal
	 *            the float val
	 */
	public TdfFloat(Float floatVal) {
		mFloatVal = floatVal;
	}
	
	/**
	 * Instantiates a new tdf float.
	 * 
	 * @param floatVal
	 *            the float val
	 */
	public TdfFloat(float floatVal) {
		mFloatVal = new Float(floatVal);
	}
	
	/**
	 * Gets the value
	 * 
	 * @return the float
	 */
	public Float get() {
		return mFloatVal;
	}
	
	/**
	 * Sets the value
	 * 
	 * @param floatVal
	 *            the float val
	 */
	public void set(Float floatVal) {
		mFloatVal = floatVal;
	}
	
	/**
	 * Float value.
	 * 
	 * @return the float
	 */
	public float floatValue() { 
		return mFloatVal.floatValue();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mFloatVal.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mFloatVal.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfFloat o) {
	    return mFloatVal.compareTo(o.get());
    }
}
