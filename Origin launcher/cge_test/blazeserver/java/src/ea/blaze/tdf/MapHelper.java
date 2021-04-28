/**
 *  MapHelper.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.TreeMap;
import java.util.Set;

/**
 * This interface is used by Auto-generated TDF code to provide visitors with information about map types as well 
 * as a mechanism to transparently visit the map members without having to know the types enclosed in the map.
 */
public interface MapHelper {
	
	/**
	 * Used in auto-generated code when the initializeMap methods are made to help visitMembers correctly visit the map.
	 */
	public static enum MapInitCategory {
		
		CAPACITY_INIT,
		KEYSET_INIT,
		NOT_INITIALIZED
	};
	
	/**
	 * Gets the key type for this map.
	 * 
	 * @return the key type
	 */
	public BaseType getKeyType();
	
	/**
	 * Gets the value type for this map.
	 * 
	 * @return the value type
	 */
	public BaseType getValueType();
	
	/**
	 * Initialize map with empty values for all keys in keySet.
	 * 
	 * @param map
	 *            the map to initialize
	 * @param keySet
	 *            the set containing all keys to initialize map with.
	 */
	public void initializeMap(TreeMap<?,?> map, Set<?> keySet);
	
	/**
	 * Initialize map with empty key/value pairs to match capacity.
	 * 
	 * @param map
	 *            the map to initialize
	 * @param capacity
	 *            the number of members to initialize map with.
	 */
	public void initializeMap(TreeMap<?,?> map, int capacity);
	
	/**
	 * Visit members of the map and pass them to the visitor.
	 * 
	 * @param visitor
	 *            The visitor to pass each key/value pair to.
	 * @param rootTdf
	 *            The root tdf to forward on to the visitor
	 * @param parentTdf
	 *            The parent tdf to forward on to the visitor
	 * @param tag
	 *            The tag of the map being visited.
	 * @param mapValue
	 *            The map being visited.
	 * @param referenceValue
	 *            The reference map being visited.
	 */
	public void visitMembers(Visitor visitor, Tdf rootTdf, Tdf parentTdf, long tag, TreeMap<?,?> mapValue, TreeMap<?,?> referenceValue);
}
