/**
 *  TdfString.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

import java.util.Comparator;

/**
 * Simple wrapper class for Tdf Strings
 */
public class TdfString implements Comparable<TdfString> {
	
	public final static Comparator<Object> TDF_STRING_IGNORE_CASE_CMP = new Comparator<Object>() {
		public int compare(Object obj1, Object obj2) {
			TdfString str1 = (TdfString)obj1;
			TdfString str2 = (TdfString)obj2;
			return(str1.get().compareToIgnoreCase(str2.get()));
		}
	};
	
	/** The string val. */
	private String mStringVal;

	/**
	 * Instantiates a new tdf string.
	 */
	public TdfString() {
		mStringVal = new String("");
	}

	/**
	 * Instantiates a new tdf string.
	 * 
	 * @param other
	 *            the other
	 */
	public TdfString(TdfString other) {
		mStringVal = other.get();
	}

	/**
	 * Instantiates a new tdf string.
	 * 
	 * @param stringVal
	 *            the string val
	 */
	public TdfString(String stringVal) {
		mStringVal = stringVal;
	}

	/**
	 * Gets the value
	 * 
	 * @return the string
	 */
	public String get() {
		return mStringVal;
	}

	/**
	 * Sets the value.
	 * 
	 * @param stringVal
	 *            the string val
	 */
	public void set(String stringVal) {
		mStringVal = stringVal;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mStringVal.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mStringVal.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfString o) {
	    return mStringVal.compareTo(o.get());
    }
}
