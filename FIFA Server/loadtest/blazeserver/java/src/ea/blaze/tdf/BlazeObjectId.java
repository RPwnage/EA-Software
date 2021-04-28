/**
 *  BlazeObjectId.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.StringTokenizer;

/**
 * This class refers to a particular Blaze object in the context of it's owner component.
 * A BlazeObjectId consists of a BlazeObjectType and a unique value identifying the instance
 * of the given type.
 */
public class BlazeObjectId implements Comparable<BlazeObjectId>
{
	
	/** Represents an invalid object id */
	final static BlazeObjectId BLAZE_OBJECT_ID_INVALID  = new BlazeObjectId(0, 0, 0);
	
	/** The entity id. */
	private long mEntityId;
	
	/** The entity type. */
	private BlazeObjectType mType;
	
	/**
	 * Instantations a BlazeObjectId with all values set to the provided default
	 * @param defaultValues Default values for all fields.
	 */
	public BlazeObjectId(long defaultValues) {
	
		mType = new BlazeObjectType((int)defaultValues, (int)defaultValues);
		mEntityId = defaultValues;
	}
	
	/**
	 * Instantiates a new blaze object id.
	 */
	public BlazeObjectId() {
		mType = new BlazeObjectType();
		mEntityId = 0;
	}
	
	/**
	 * Instantiates a new blaze object id.
	 * 
	 * @param rhs
	 *            the rhs
	 */
	public BlazeObjectId(BlazeObjectId rhs) {
		mType = new BlazeObjectType(rhs.mType);
		mEntityId = rhs.mEntityId;
	}
	
	/**
	 * Instantiates a new blaze object id.
	 * 
	 * @param componentId
	 *            the component id
	 * @param entityType
	 *            the entity type
	 * @param entityId
	 *            the entity id
	 */
	public BlazeObjectId(int componentId, int entityType, long entityId) {
    	mType = new BlazeObjectType(componentId, entityType);
    	mEntityId = entityId;
    }
   
    /**
	 * Instantiates a new blaze object id.
	 * 
	 * @param t
	 *            the t
	 * @param entityId
	 *            the entity id
	 */
    public BlazeObjectId(BlazeObjectType t, long entityId) {
    	mEntityId = entityId;
    	mType = new BlazeObjectType(t);
    }
    
    /**
	 * Sets the.
	 * 
	 * @param rhs
	 *            the rhs
	 */
    public void set(BlazeObjectId rhs) {
    	this.mEntityId = rhs.mEntityId;
    	this.mType.set(rhs.getType());
    }

    /**
	 * Sets the entity id.
	 * 
	 * @param entityId
	 *            the new entity id
	 */
    public void setEntityId(long entityId) {
    	this.mEntityId = entityId;
    }
    
    /**
	 * Gets the entity id.
	 * 
	 * @return the entity id
	 */
    public long getEntityId() {
    	return this.mEntityId;
    }
    
    /**
	 * Sets the entity type.
	 * 
	 * @param rhs
	 *            the new entity type
	 */
    public void setType(BlazeObjectType rhs) {
    	this.mType.set(rhs);
    }
        
    /**
	 * Gets the entity type.
	 * 
	 * @return the entity type
	 */
    public BlazeObjectType getType() {
    	return this.mType;
    }
  
    /**
	 * Equals.
	 * 
	 * @param rhs
	 *            the rhs
	 * @return true, if successful
	 */
    public boolean equals(Object rhs) {
        return ((rhs instanceof BlazeObjectId) && (mEntityId == ((BlazeObjectId)rhs).mEntityId) 
                && (mType.equals(((BlazeObjectId)rhs).mType)));
    }
    
    /**
	 * Returns a hash code value for the object.
	 * 
	 * @return a hash code value for the object
	 */
    public int hashCode() 
    { 
        return (int)(mEntityId ^ (mEntityId >>> 32)) ^ mType.hashCode(); 
    }
    
    /* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    public int compareTo(BlazeObjectId rhs) {
        int  diff = (int)mType.getComponentId() - (int)rhs.mType.getComponentId();
        if (diff < 0)
            return -1;
        if (diff > 0)
            return 1;
        diff = (int)mType.getTypeId() - (int)rhs.mType.getTypeId();
        if (diff < 0)
            return -1;
        if (diff > 0)
            return 1;
        
        return (new Long(mEntityId).compareTo(rhs.mEntityId));
    }
    
    /**
	 * To string.
	 * 
	 * @param separator
	 *            the separator
	 * @return the string
	 */
    public String toString(char separator) {
    	return mType.toString() + separator + Long.toString(mEntityId);
    }
    
    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    public String toString() {
    	return toString('/');
    }

	/**
	 * From string.
	 * 
	 * @param string
	 *            the string
	 * @return the blaze object id
	 */
	public static BlazeObjectId fromString(String string) {
		StringTokenizer tok = new StringTokenizer(string, "/");
		if(tok.countTokens() == 3) {
			String compIdStr = tok.nextToken();
			String typeIdStr = tok.nextToken();
			String entityIdStr = tok.nextToken();
			try {
				return new BlazeObjectId(Integer.parseInt(compIdStr), Integer.parseInt(typeIdStr), Long.parseLong(entityIdStr));
			}
			catch(NumberFormatException ex) {
				return null;
			}
		}
		return null;
	}
}
