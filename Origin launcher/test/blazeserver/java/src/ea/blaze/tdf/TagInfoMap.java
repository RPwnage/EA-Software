/**
 *  TagInfoMap.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * TagInfoMap contains call TagInfo values for a TDF class.  Contains interface to retrieve the information
 * for a specified TDF member by the member name, the member tag, or the member index.
 */
public class TagInfoMap 
{
	
	/** The array tag info for the Tdf class. */
	private TagInfo[] mTagInfo;

	
	/**
	 * Instantiates a new tag info map from the array of TagInfo.
	 * 
	 * @param tagInfo
	 *            the tag info array to instantiate this TagInfoMap with.
	 */
	public TagInfoMap(TagInfo[] tagInfo) {
		mTagInfo = tagInfo;
	}
	
	/**
	 * Gets the member with the specified tag, or none if it does not exist.
	 * 
	 * @param tag
	 *            the tag to search for
	 * @return the tag info for the member with the specified tag, or null if no member was found.
	 */
	public TagInfo get(long tag) {
		for(int i = 0; i < mTagInfo.length; i++) {
			if(mTagInfo[i].getTag() == tag)
				return mTagInfo[i];
		}
		
		return null;
	}
	
	/**
	 * Find tag info for member with the specified name.
	 * 
	 * @param member
	 *            the member name to find.
	 * @return the tag info for the specified member, or null if no member was found.
	 */
	public TagInfo find(String member) {
		for(int i = 0; i < mTagInfo.length; i++) {
			if(mTagInfo[i].getMember().compareToIgnoreCase(member) == 0)
				return mTagInfo[i];
		}
		
		return null;
	}
	
	/**
	 * Gets the tag for the member at the specified index.
	 * 
	 * @param index
	 *            the index for the member to fetch.
	 * @return the tag info for the member at index, or null if no member was found.
	 */
	public TagInfo getTagAtIndex(int index) {
		if(index >= 0 && index < mTagInfo.length) {
			return mTagInfo[index];
		}
		else {
			return null;
		}
	}
}
