/**
 *  BlazeObjectType.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.StringTokenizer;

/**
 * This class refers to a particular Blaze type in the context of it's owner component.
 * A BlazeObjectType consists of a component ID and an entity type which is unique to the
 * component itself.
 */
public class BlazeObjectType implements Comparable<BlazeObjectType>
{
	
	/** The component id. */
	private int mComponentId = 0;
	
	/** The entity type id. */
	private int mTypeId = 0;
	
	/**
	 * Instantiates a new blaze object type.
	 */
	public BlazeObjectType() {
		
	}
	
	/**
	 * Instantiates a new blaze object type.
	 * 
	 * @param val
	 *            the val
	 */
	public BlazeObjectType(long val) {
		set((int) ((val >> 16) & 0xffff), (int) val);
	}
	
	/**
	 * Instantiates a new blaze object type.
	 * 
	 * @param componentId
	 *            the component id
	 * @param typeId
	 *            the type id
	 */
	public BlazeObjectType(int componentId, int typeId) {
		set(componentId, typeId);
	}
	
	/**
	 * Instantiates a new blaze object type.
	 * 
	 * @param rhs
	 *            the rhs
	 */
	public BlazeObjectType(BlazeObjectType rhs) {
		set(rhs.getComponentId(), rhs.getTypeId());
	}
	
	/**
	 * To long.
	 * 
	 * @return the long
	 */
	public long toLong() { return ((mComponentId << 16) + mTypeId) & 0xffffffff; }
	
    /**
	 * Equals.
	 * 
	 * @param rhs
	 *            the rhs
	 * @return true, if successful
	 */
    public boolean equals(BlazeObjectType rhs) {
        return ((rhs instanceof BlazeObjectType) && (mComponentId == ((BlazeObjectType)rhs).mComponentId) 
                && (mTypeId == ((BlazeObjectType)rhs).mTypeId));
    }
    
    /**
	 * Returns a hash code value for the object.
	 * 
	 * @return a hash code value for the object
	 */
    public int hashCode() 
    { 
        return mComponentId ^ mTypeId; 
    }

    /* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    public int compareTo(BlazeObjectType rhs) {
    	if (toLong() < rhs.toLong())
    		return -1;
    	if (toLong() > rhs.toLong())
    		return 1;
    	else
    		return 0;
    }
    
    /**
	 * To string.
	 * 
	 * @param separator
	 *            the separator
	 * @return the string
	 */
    public String toString(char separator) {
        String str = "";
        str = String.format("%d%c%d", mComponentId, separator, mTypeId);
        return str;
    }
    
    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    public String toString() {
    	return toString('/');
    }
    
	/**
	 * Gets the component id.
	 * 
	 * @return the component id
	 */
	public int getComponentId() {
		return mComponentId;
	}
	
	/**
	 * Gets the type id.
	 * 
	 * @return the type id
	 */
	public int getTypeId() {
		return mTypeId;
	}
	
	/**
	 * Sets the component id.
	 * 
	 * @param newComponentId
	 *            the new component id
	 */
	public void setComponentId(int newComponentId) {
		mComponentId = newComponentId;
	}
	
	/**
	 * Sets the type id.
	 * 
	 * @param newTypeId
	 *            the new type id
	 */
	public void setTypeId(int newTypeId) {
		mTypeId = newTypeId;
	}
	
	/**
	 * Sets the.
	 * 
	 * @param newComponentId
	 *            the new component id
	 * @param newTypeId
	 *            the new type id
	 */
	public void set(int newComponentId, int newTypeId) {
		mComponentId = newComponentId;
		mTypeId = newTypeId;
	}
	
	/**
	 * Sets the.
	 * 
	 * @param rhs
	 *            the rhs
	 */
	public void set(BlazeObjectType rhs) {
		mComponentId = rhs.getComponentId();
		mTypeId = rhs.getTypeId();
	}

	/**
	 * From string.
	 * 
	 * @param string
	 *            the string
	 * @return the blaze object type
	 */
	public static BlazeObjectType fromString(String string) {
		StringTokenizer tok = new StringTokenizer(string, "/");
		if(tok.countTokens() == 2) {
			String compIdStr = tok.nextToken();
			String typeIdStr = tok.nextToken();
			
			try {
				return new BlazeObjectType(Integer.parseInt(compIdStr), Integer.parseInt(typeIdStr));
			}
			catch(NumberFormatException ex) {
				return null;
			}
		}
		return null;
	}

}
