/**
 *  TdfEnum.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;


/**
 * The Interface TdfEnum, provides interfaces to interact with TDF enum members.
 * Auto-generated TDF enums all implement this interface.
 */
public interface TdfEnum {
	
	/**
	 * Gets the value of the enum
	 * 
	 * @return the value
	 */
	public int getValue();
	
	/**
	 * Gets the name of the enum
	 * 
	 * @return the name
	 */
	public String getName();
	
	/**
	 * Lookup enum by value.
	 * 
	 * @param value
	 *            the value to lookup
	 * @return the tdf enum if one is available with value, null otherwise.
	 */
	public TdfEnum lookupValue(int value);
    
    /**
	 * Lookup enum by name.
	 * 
	 * @param name
	 *            the name to lookup
	 * @return the tdf enum if one is available with name, null otherwise.
	 */
    public TdfEnum lookupName(String name);
}
