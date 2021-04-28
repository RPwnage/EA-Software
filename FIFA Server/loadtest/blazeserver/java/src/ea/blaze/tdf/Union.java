/**
 *  Union.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * The base Union class for TDF unions
 */
public abstract class Union extends Tdf
{
	
	/** The Constant INVALID_MEMBER_INDEX. */
	public static final int INVALID_MEMBER_INDEX = 127;
	
	/**
	 * Gets the active member.
	 * 
	 * @return the active member
	 */
	public abstract int getActiveMember();
	
	/**
	 * Switch active member.
	 * 
	 * @param member
	 *            the member
	 */
	public abstract void switchActiveMember(int member);
	
	/**
	 * Valid member.
	 * 
	 * @param member
	 *            the member
	 * @return true, if successful
	 */
	public abstract boolean validMember(int member);
	
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Tdf#visit(ea.blaze.tdf.Visitor, ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public abstract void visit(Visitor visitor, Tdf rootTdf, Tdf referenceTdf);
	
}
