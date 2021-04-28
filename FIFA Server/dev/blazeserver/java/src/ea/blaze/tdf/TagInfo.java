/**
 *  TagInfo.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * The TagInfo class provides information about a TDF member variable, including it's native C++ type.
 */
public class TagInfo 
{	
	
	/**
	 * The native C++ type of the value.  In cases of numeric values, this can be important to track unsigned values
	 * that needed to be promoted to next larger numeric value type in Java since Java does not have unsigned types.
	 */
	public enum NativeType
	{
		INT8,
		INT16,
		INT32,
		INT64,
		UINT8,
		UINT16,
		UINT32,
		UINT64,
		STRING,
    	BINARY,
    	STRUCT,
    	LIST,
    	MAP,
    	UNION,
    	VARIABLE,
    	BLAZE_OBJECT_TYPE,
    	BLAZE_OBJECT_ID,
		FLOAT,
    	TIMEVALUE
	};
	
	/** The tag of the TDF member */
	private long mTag;
	
	/** The member index of the TDF member. */
	private long mMemberIndex;
	
	/** The member name */
	private String mMember;
	
	/** The description of the member */
	private String mDescription;
	
	/** The default value for the member */
	private String mDefault;
	
	/** The member native type. */
	private NativeType mMemberNativeType;
	
	/**
	 * Static method to decode long based tag into 4 character strings as represented in TDF files.
	 * 
	 * @param tag
	 *            the tag to decode
	 * @param convertToLowercase
	 *            whether to convert the decoded tag to lowercase
	 * @return the decoded tag string.
	 */
	public static String decodeTag(long tag, boolean convertToLowercase) {
		byte[] buf = new byte[5];
	    int size = 4;
	    tag >>= 8;
	    for (int i = 3; i >= 0; --i) {
	        long sixbits = tag & 0x3f;
	        if (sixbits != 0) {
	            buf[i] = (byte)(sixbits + 32);
	        }
	        else {
	            buf[i] = '\0';
	            size = i;
	        }
	        tag >>= 6;
	    }

	    if (convertToLowercase) {
	        for (int i = 0 ; i < size ; ++i) {
	            buf[i] = (byte)Character.toLowerCase(buf[i]);
	        }
	    }
	    
	    /* Find the length of the non-NULL characters. */
    	int actualLength;
    	for (actualLength = 0; actualLength < buf.length; actualLength++) {
    		if (buf[actualLength] == 0)
    			break;
    	}

    	return new String(buf, 0, actualLength);
	}
	
	/**
	 * Instantiates a new tag info for a TDF member.
	 * 
	 * @param type
	 *            the type
	 * @param tag
	 *            the tag
	 * @param member
	 *            the member
	 * @param index
	 *            the index
	 * @param description
	 *            the description
	 * @param def
	 *            the def
	 */
	public TagInfo(NativeType type, long tag, String member, long index, String description, String def)
	{
		mMemberNativeType = type;
		mTag = tag;
		mMemberIndex = index;
		mMember = member;
		mDescription = description;
		mDefault = def;
	}
	
	/**
	 * Gets the TDF member native type.
	 * 
	 * @return the TDF member native type
	 */
	public NativeType getMemberNativeType() { return mMemberNativeType; }
	
	/**
	 * Gets the TDF member tag.
	 * 
	 * @return the TDF member tag
	 */
	public long getTag()  { return mTag; }
	
	/**
	 * Gets the TDF member index.
	 * 
	 * @return the TDF member index
	 */
	public long getMemberIndex() { return mMemberIndex; }
	
	/**
	 * Gets the TDF member name.
	 * 
	 * @return the TDF member name
	 */
	public String getMember() { return mMember; }
	
	/**
	 * Gets the TDF member description.
	 * 
	 * @return the TDF member description
	 */
	public String getDescription() { return mDescription; }
	
	/**
	 * Gets the TDF member default value.
	 * 
	 * @return the TDF member default value
	 */
	public String getDefault() { return mDefault; }
}
