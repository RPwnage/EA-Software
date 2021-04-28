/**
 *  TdfCopier.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.TreeMap;
import java.util.Vector;

import ea.blaze.tdf.types.*;

/**
 * A visitor implementation that copies each member of one TDF to the other.
 */
public class TdfCopier implements Visitor {

	/** A static TdfCopier for the short-circuit copyTdf static method. */
	private static TdfCopier sCopier = new TdfCopier();
	
	/**
	 * Static method to copy source TDF into destination.
	 * 
	 * @param destination
	 *            the destination of the copy
	 * @param source
	 *            the source of the copy
	 */
	public static void copyTdf(Tdf destination, Tdf source) {
		sCopier.visit(destination, source);
	}
	
	/**
	 * Non-static method to copy source TDF into destination.
	 * 
	 * @param destination
	 *            the destination of the copy
	 * @param source
	 *            the source of the copy
	 */
	public void copy(Tdf destination, Tdf source) {
		visit(destination, source);
	}
	
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf tdf, Tdf referenceValue) {
		tdf.visit(this, tdf, referenceValue);
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Union tdf, Union referenceValue) {
		tdf.switchActiveMember(referenceValue.getActiveMember());
		tdf.visit(this, tdf, referenceValue);
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.Vector, java.util.Vector, ea.blaze.tdf.VectorHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value,
			Vector<?> referenceValue, VectorHelper memberHelper) {
		memberHelper.initializeVector(value, referenceValue.size());
		memberHelper.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.TreeMap, java.util.TreeMap, ea.blaze.tdf.MapHelper)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			TreeMap<?, ?> value, TreeMap<?, ?> referenceValue,
			MapHelper memberHelper) {
		memberHelper.initializeMap(value, referenceValue.keySet());
		memberHelper.visitMembers(this, roottdf, parentTdf, tag, value, referenceValue);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfBoolean, ea.blaze.tdf.types.TdfBoolean, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfBoolean value,
			TdfBoolean referenceValue, boolean defaultValue) {
		value.set(referenceValue.get());
	}
            
    /* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfChar, ea.blaze.tdf.types.TdfChar, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfChar value,
			TdfChar referenceValue, char defaultValue) {
        value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfByte, ea.blaze.tdf.types.TdfByte, byte)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfByte value,
			TdfByte referenceValue, byte defaultValue) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfShort, ea.blaze.tdf.types.TdfShort, short)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfShort value,
			TdfShort referenceValue, short defaultValue) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, int)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, int defaultValue) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfLong, ea.blaze.tdf.types.TdfLong, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfLong value,
			TdfLong referenceValue, long defaultValue) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfUInt64 value,
			TdfUInt64 referenceValue, TdfUInt64 defaultValue) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfFloat, ea.blaze.tdf.types.TdfFloat, float)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfFloat value,
			TdfFloat referenceValue, float defaultValue) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Bitfield, ea.blaze.tdf.Bitfield)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Bitfield value,
			Bitfield referenceValue) {
		value.setBits(referenceValue.getBits());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfString, ea.blaze.tdf.types.TdfString, java.lang.String, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfString value,
			TdfString referenceValue, String defaultValue, long maxLength) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Blob, ea.blaze.tdf.Blob)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Blob value,
			Blob referenceValue) {
		value.setData(referenceValue.getData());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, java.lang.Class, java.lang.Enum)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, TdfEnum enumHelper, TdfEnum defaultValue) {
		value.set(referenceValue.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf roottdf, Tdf parentTdf, long tag, Tdf value,
			Tdf referenceValue) {
		value.visit(this, roottdf, referenceValue);
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Tdf roottdf, Tdf parentTdf, long tag, Union value,
			Union referenceValue) {
		value.switchActiveMember(referenceValue.getActiveMember());
		value.visit(this, roottdf, referenceValue);
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.VariableTdfContainer, ea.blaze.tdf.VariableTdfContainer)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag,
			VariableTdfContainer value, VariableTdfContainer referenceValue) {
		Tdf referenceTdf = referenceValue.get();
		if(referenceTdf != null) {
			Tdf newTdf = TdfFactory.get().createTdf(referenceTdf.getTdfId());
			if(newTdf != null) {
				newTdf.visit(this, rootTdf, referenceTdf);
				value.set(newTdf);
			}
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectType, ea.blaze.tdf.BlazeObjectType)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectType value, BlazeObjectType referenceValue) {
		value.set(referenceValue);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectId, ea.blaze.tdf.BlazeObjectId)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectId value, BlazeObjectId referenceValue) {
		value.set(referenceValue);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TimeValue value,
			TimeValue referenceValue, TimeValue defaultValue) {
		value.set(referenceValue);
	}

}
