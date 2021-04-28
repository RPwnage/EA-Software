/**
 *  TdfChar.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

/**
 * Simple wrapper class for TDF Character values.
 */
public class TdfChar implements Comparable<TdfChar> {
	
	/** The Character val. */
	private Character mCharVal;
	
	/**
	 * Instantiates a new tdf Character.
	 */
	public TdfChar() {
		mCharVal = new Character('\0');
	}
	
	/**
	 * Instantiates a new tdf Character.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfChar(TdfChar other) {
		mCharVal = other.get();
	}
	
	/**
	 * Instantiates a new tdf Character.
	 * 
	 * @param charVal
	 *            the Character val
	 */
	public TdfChar(Character charVal) {
		mCharVal = charVal;
	}
    
    	/**
	 * Instantiates a new tdf Character.
	 * 
	 * @param charVal
	 *            the Character val
	 */
	public TdfChar(char charVal) {
		mCharVal = new Character(charVal);
	}
	
	/**
	 * Gets the value
	 * 
	 * @return the Character
	 */
	public Character get() {
		return mCharVal;
	}
	
	/**
	 * Sets the value
	 * 
	 * @param charVal
	 *            the Character val
	 */
	public void set(Character charVal) {
		mCharVal = charVal;
	}
	
	/**
	 * Character value.
	 * 
	 * @return the Character value
	 */
	public Character charValue() { 
		return mCharVal.charValue();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mCharVal.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mCharVal.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfChar o) {
	    return mCharVal.compareTo(o.get());
    }
}
