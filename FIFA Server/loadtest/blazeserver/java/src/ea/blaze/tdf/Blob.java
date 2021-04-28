/**
 *  Blob.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * Simple blob class used by the tdf types to carry variable length binary data.
 */
public class Blob 
{
	
	/** The binary data. */
	private byte mData[];
	
	/**
	 * Gets the binary data.
	 * 
	 * @return the binary data
	 */
	public byte[] getData() 
	{
		return mData;
	}
	
	/**
	 * Sets the binary data.
	 * 
	 * @param value
	 *            the binary data
	 * @return true, if data set successfully
	 */
	public boolean setData(byte value[])
	{
		mData = value.clone();
		return true;
	}
	
	/**
	 * Gets the binary data size.
	 * 
	 * @return the size
	 */
	public long getSize() { return mData.length; }
		
	/**
	 * Instantiates a new blob with an empty binary data.
	 */
	public Blob()
	{
		mData = new byte[0];
	}
	
	/**
	 * Instantiates a new blob and copies existing blob data.
	 * 
	 * @param rhs
	 *            the blob to copy data from.
	 */
	public Blob(Blob rhs) {
		setData(rhs.getData());
	}
}
