/**
 *  VariableTdfContainer.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * The VariableTdfContainer for variable TDF member fields.  Holds a TDF class, or null
 * if variable member field was empty.
 */
public class VariableTdfContainer
{	
	
	/** The TDF */
	protected Tdf mTdf;

	/**
	 * Instantiates a new empty variable TDF container.
	 */
	public VariableTdfContainer() {
		mTdf = null;
	}
	
	/**
	 * Instantiates a new variable tdf container and copies TDF from rhs container.
	 * 
	 * @param rhs
	 *            the rhs variable container to copy.
	 */
	public VariableTdfContainer(VariableTdfContainer rhs) {
		mTdf = rhs.get();
	}
	
	/**
	 * Clears this container, removing the contained TDF.
	 */
	public void clear() {
		mTdf = null;
	}
	
	/**
	 * Checks if is this container is valid, where valid means an actual TDF is contained.
	 * 
	 * @return true, if this container contains a valid TDF.
	 */
	public boolean isValid() { 
		return (mTdf != null);
	}
	
	/**
	 * Gets the TDF, or null if no TDF is present.
	 * 
	 * @return the contained TDF, or null if no TDF is present.
	 */
	public Tdf get() { 
		return mTdf;
	}
	
	/**
	 * Sets the contained TDF to the passed in TDF.
	 * 
	 * @param tdf
	 *            the tdf to store in this container.
	 */
	public void set(Tdf tdf) {
		mTdf = tdf;
	}
}
