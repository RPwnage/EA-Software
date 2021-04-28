/**
 *  Bitfield.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * The base class for TDF Bitfields.
 */
public class Bitfield 
{
	
	/**
	 * Sets the bits.
	 * 
	 * @param bits
	 *            the new bits
	 */
	public void setBits(long bits){ mBits = bits; }
	
	/**
	 * Gets the bits.
	 * 
	 * @return the bits
	 */
	public long getBits() { return mBits; }
	
	/** The bits. */
	protected long mBits;
}
