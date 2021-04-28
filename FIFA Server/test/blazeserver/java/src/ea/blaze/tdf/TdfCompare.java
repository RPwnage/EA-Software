/**
 *  TdfCompare.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.TreeMap;
import java.util.Vector;

import ea.blaze.tdf.types.*;

/**
 * A visitor implementation that compares each member of two TDFs recursively.
 */
public class TdfCompare implements Visitor {

	/** The compare result. */
	private boolean mCompareResult = true;
	
	/** A static TdfCompare for the short-circuit compareTdfs static method. */
	private static TdfCompare sTdfCompare = new TdfCompare();
	
	/**
	 * Static method to compare two TDFs.
	 * 
	 * @param lhs
	 *            the lhs TDF to compare
	 * @param rhs
	 *            the rhs TDF to compare
	 * @return true, if TDFs are equal, false otherwise.
	 */
	public static boolean compareTdfs(Tdf lhs, Tdf rhs) {
		return sTdfCompare.equals(lhs, rhs);
	}
	
	/**
	 * Non-static method to compare two TDFs.
	 * 
	 * @param lhs
	 *            the lhs TDF to compare
	 * @param rhs
	 *            the rhs TDF to compare
	 * @return true, if TDFs are equal, false otherwise.
	 */
	public boolean equals(Tdf lhs, Tdf rhs) {
		if(lhs.getClass() == rhs.getClass()) {
			mCompareResult = true;
			lhs.visit(this, lhs, rhs);
			return mCompareResult;
		}
		else {
			return false;
		}
	}
	
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf tdf, Tdf referenceValue) {
		if(mCompareResult) {
			if(tdf.getClass() == referenceValue.getClass()) {
				tdf.visit(this, tdf, referenceValue);
			}
			else {
				mCompareResult = false;
			}
		}
		
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Union tdf, Union referenceValue) {
		if(mCompareResult) {
			if(tdf.getActiveMember() == referenceValue.getActiveMember()) {
				tdf.visit(this, tdf, referenceValue);
			} 
			else {
				mCompareResult = false;
			}
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.Vector, java.util.Vector, ea.blaze.tdf.VectorHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value,
			Vector<?> referenceValue, VectorHelper memberHelper) {
		if(mCompareResult) {
			if(value.size() == referenceValue.size()) {
				memberHelper.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
			}
			else {
				mCompareResult = false;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.TreeMap, java.util.TreeMap, ea.blaze.tdf.MapHelper)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			TreeMap<?, ?> value, TreeMap<?, ?> referenceValue, MapHelper memberHelper) {
		if(mCompareResult) {
			if(value.size() == referenceValue.size()) {
				memberHelper.visitMembers(this, roottdf, parentTdf, tag, value, referenceValue);
			}
			else {
				mCompareResult = false;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfBoolean, ea.blaze.tdf.types.TdfBoolean, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfBoolean value,
			TdfBoolean referenceValue, boolean defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}
    
    /* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfChar, ea.blaze.tdf.types.TdfChar, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfChar value,
			TdfChar referenceValue, char defaultValue) {
        mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfByte, ea.blaze.tdf.types.TdfByte, byte)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfByte value,
			TdfByte referenceValue, byte defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfShort, ea.blaze.tdf.types.TdfShort, short)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfShort value,
			TdfShort referenceValue, short defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, int)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, int defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfLong, ea.blaze.tdf.types.TdfLong, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfLong value,
			TdfLong referenceValue, long defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfUInt64 value,
			TdfUInt64 referenceValue, TdfUInt64 defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfFloat, ea.blaze.tdf.types.TdfFloat, float)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfFloat value,
			TdfFloat referenceValue, float defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Bitfield, ea.blaze.tdf.Bitfield)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Bitfield value,
			Bitfield referenceValue) {
		mCompareResult = mCompareResult && (value.getBits() == referenceValue.getBits());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfString, ea.blaze.tdf.types.TdfString, java.lang.String, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfString value,
			TdfString referenceValue, String defaultValue, long maxLength) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Blob, ea.blaze.tdf.Blob)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Blob value,
			Blob referenceValue) {
		mCompareResult = mCompareResult && ((value.getSize() == referenceValue.getSize()) && 
			(java.util.Arrays.equals(value.getData(), referenceValue.getData())));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, java.lang.Class, java.lang.Enum)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, TdfEnum enumHelper, TdfEnum defaultValue) {
		mCompareResult = mCompareResult && (value.get().equals(referenceValue.get()));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf roottdf, Tdf parentTdf, long tag, Tdf value,
			Tdf referenceValue) {
		if(mCompareResult) {
			if(value.getClass() == referenceValue.getClass()) {
				value.visit(this, roottdf, referenceValue);
			}
			else {
				mCompareResult = false;
			}
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Tdf roottdf, Tdf parentTdf, long tag, Union value,
			Union referenceValue) {
		
		if(mCompareResult) {
			if(value.getActiveMember() == referenceValue.getActiveMember()) {
				value.visit(this, roottdf, referenceValue);
			} 
			else {
				mCompareResult = false;
			}
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.VariableTdfContainer, ea.blaze.tdf.VariableTdfContainer)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag,
			VariableTdfContainer value, VariableTdfContainer referenceValue) {
		if(mCompareResult) {
			Tdf valueTdf = value.get();
			Tdf referenceTdf = referenceValue.get();
			if(valueTdf != null && referenceTdf != null) {
				if(valueTdf.getTdfId() == referenceTdf.getTdfId()) {
					valueTdf.visit(this, rootTdf, referenceTdf);
				}
				else {
					mCompareResult = false;
				}
			}
			else if(valueTdf != referenceTdf) {
				mCompareResult = false;
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
		mCompareResult = mCompareResult && (value.equals(referenceValue));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectId, ea.blaze.tdf.BlazeObjectId)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectId value, BlazeObjectId referenceValue) {
		mCompareResult = mCompareResult && (value.equals(referenceValue));
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TimeValue value,
			TimeValue referenceValue, TimeValue defaultValue) {
		mCompareResult = mCompareResult && (value.equals(referenceValue));
	}
}
