/**
 *  VectorHelper.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.Vector;

/**
 * This interface is used by Auto-generated TDF code to provide visitors with information about vector types as well 
 * as a mechanism to transparently visit the list members without having to know the types enclosed in the vector.
 */
public interface VectorHelper {
	
	/**
	 * Gets the value type for this vector.
	 * 
	 * @return the value type
	 */
	public BaseType getValueType();
	
    /**
     * Initializes the vector with empty values up to capacity.
     * 
     * @param value The vector to initialize
     * @param capcity The number of members to initialize vector with.
     */
	public void initializeVector(Vector<?> value, int capacity);
	
    /**
     * Helper to allow visitors to visit each member of the vector.
     *
     * @param visitor Visitor to apply to each of the items in the vector.
     * @param rootTdf The Root TDF to pass through to the visit method for each item.
     * @param parentTdf The Parent TDF to pass through to the visit method for each item.
     * @param value The vector being visited.
     * @param referenceValue the reference vector being visited.
      */
	public void visitMembers(Visitor visitor, Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value, Vector<?> referenceValue);
}
