/**
 *  BaseType.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * The Enum BaseType.
 */
public enum BaseType 
{
   
    TDF_TYPE_INTEGER,
    TDF_TYPE_STRING,
    TDF_TYPE_BINARY,
    TDF_TYPE_STRUCT,
    TDF_TYPE_LIST,
    TDF_TYPE_MAP,
    TDF_TYPE_UNION,
    TDF_TYPE_VARIABLE,
    TDF_TYPE_BLAZE_OBJECT_TYPE,
    TDF_TYPE_BLAZE_OBJECT_ID,
	TDF_TYPE_FLOAT,
    TDF_TYPE_TIMEVALUE,
    TDF_TYPE_MAX;
    
    /**
	 * Checks if the integer represents a valid BaseType enum.
	 * 
	 * @param baseType
	 *            the base type to check
	 * @return true, if baseType is valid.
	 */
    public static boolean valid(int baseType) {
    	return(baseType >= 0 && baseType < TDF_TYPE_MAX.ordinal());
    }
}
