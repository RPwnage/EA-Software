/**
 *  Visitor.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.TreeMap;
import java.util.Vector;

import ea.blaze.tdf.types.TdfEnum;
import ea.blaze.tdf.types.TdfBoolean;
import ea.blaze.tdf.types.TdfChar;
import ea.blaze.tdf.types.TdfByte;
import ea.blaze.tdf.types.TdfFloat;
import ea.blaze.tdf.types.TdfInteger;
import ea.blaze.tdf.types.TdfLong;
import ea.blaze.tdf.types.TdfShort;
import ea.blaze.tdf.types.TdfString;
import ea.blaze.tdf.types.TdfUInt64;

/**
 * Classes implementing this interface will be able to visit each member of a TDF.  This includes
 * nested members such as members that are also TDFs, vectors, maps, unions, etc.
 */
public interface Visitor 
{
	
	/**
	 * This is the main entrance to a visitor.  The TDF to visit and a reference TDF which
	 * provides a way to provide a second read-only TDF with reference information for visitors
	 * such as TdfCompare or TdfCopy.  This passes the control the TDF which in turn calls the
	 * visit method for each of it's members on this visitor.
	 * 
	 * The reference TDF can be the same as the main TDF.
	 * 
	 * @param tdf
	 *            the tdf to visit
	 * @param referenceValue
	 *            the reference value tdf to visit
	 * @return true, if this visit determines the visit was successful based on case-by-case criteria.
	 */
	public boolean visit(Tdf tdf, Tdf referenceValue);
    
    /**
	 * Allows visiting the specific union and it's members.  Technically a union is a TDF class
	 * but only has one active member set and that information can be used by a visitor to short
	 * circuit a visitation if the both values don't have the same active member set.
	 * 
	 * @param union
	 *            the union to visit. 
	 * @param referenceUnion
	 *            the reference union to visit
	 * @return true, if this visit determines the visit was successful based on case-by-case criteria
	 */
    public boolean visit(Union union, Union referenceUnion);

	/**
	 * Visits a list based member of the TDF.
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value for this member
	 * @param referenceValue
	 *            the reference value for this member
	 * @param memberHelper
	 *            the vector helper class to provide methods to visit each item in the vector 
	 */
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value, Vector<?> referenceValue, VectorHelper memberHelper);
	
	/**
	 * Visit a map based member of the TDF.
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value for this member
	 * @param mapVisitor
	 *            the map helper class to provide methods to visit each item in the vector.
	 */
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TreeMap<?,?> value, TreeMap<?,?> referenceValue, MapHelper mapVisitor);

    /**
	 * Visit a member of the TDF of type boolean
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped boolean value of this member
	 * @param referenceValue
	 *            the wrapped boolean reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfBoolean value, TdfBoolean referenceValue, boolean defaultValue);
     
     /**
	 * Visit a member of the TDF of type char
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped boolean value of this member
	 * @param referenceValue
	 *            the wrapped boolean reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfChar value, TdfChar referenceValue, char defaultValue);
    
    /**
	 * Visit a member of the TDF of type byte
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped byte value of this member
	 * @param referenceValue
	 *            the wrapped byte reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfByte value, TdfByte referenceValue, byte defaultValue);
    
    /**
	 * Visit a member of the TDF of type short
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped short value of this member
	 * @param referenceValue
	 *            the wrapped short reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfShort value, TdfShort referenceValue, short defaultValue);
    
    /**
	 * Visit a member of the TDF of type int
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped int value of this member
	 * @param referenceValue
	 *            the wrapped int reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfInteger value, TdfInteger referenceValue, int defaultValue);
    
    /**
	 * Visit a member of the TDF of type long
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped long value of this member
	 * @param referenceValue
	 *            the wrapped long reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfLong value, TdfLong referenceValue, long defaultValue);
    
    /**
	 * Visit a member of the TDF of type TdfUint64
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfUInt64 value, TdfUInt64 referenceValue, TdfUInt64 defaultValue);
    
    /**
	 * Visit a member of the TDF of type float
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped float value of this member
	 * @param referenceValue
	 *            the wrapped float reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfFloat value, TdfFloat referenceValue, float defaultValue);
	
    /**
	 * Visit a member of the TDF of type Bitfield
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Bitfield value, Bitfield referenceValue);
    
    /**
	 * Visit a member of the TDF of type string
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped string value of this member
	 * @param referenceValue
	 *            the wrapped string reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 * @param maxLength 
	 *            the maximum valid length for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfString value, TdfString referenceValue, String defaultValue, long maxLength);
    
    /**
	 * Visit a member of the TDF of type Blob
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 * @param defaultValue
	 *            the default value for this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Blob value, Blob referenceValue);

    /**
	 * Visit a member of the TDF of type enum
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the wrapped value of this member enum 
	 * @param referenceValue
	 *            the wrapped reference value of this member enum
	 * @param enumHelper
	 *            the enum helper containing methods to lookup enum by name or value.
	 * @param defaultValue
	 *            the default value for this enum
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfInteger value, TdfInteger referenceValue, TdfEnum enumHelper, TdfEnum defaultValue);
    
    /**
	 * Visit a member of the TDF of type TDF
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 * @return true, if successful visit of TDF was completed.
	 */
	 public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Tdf value, Tdf referenceValue);
    
    /**
	 * Visit a member of the TDF of type Union
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 * @return true, if successful visit of Union was completed.
	 */
	 public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Union value, Union referenceValue);
    
    /**
	 * Visit a member of the TDF of type VariableTdfContainer
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 * @return true, if successful visit of variable TDF was completed.
	 */
	 public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, VariableTdfContainer value, VariableTdfContainer referenceValue);

    /**
	 * Visit a member of the TDF of type BlazeObjectType
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, BlazeObjectType value, BlazeObjectType referenceValue);
    
    /**
	 * Visit a member of the TDF of type BlazeObjectId
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, BlazeObjectId value, BlazeObjectId referenceValue);

    /**
	 * Visit a member of the TDF of type TimeValue
	 * 
	 * @param rootTdf
	 *            the root TDF
	 * @param parentTdf
	 *            the parent TDF
	 * @param tag
	 *            the tag of this member
	 * @param value
	 *            the value of this member
	 * @param referenceValue
	 *            the reference value of this member
	 */
	 public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TimeValue value, TimeValue referenceValue, TimeValue defaultValue);
}
