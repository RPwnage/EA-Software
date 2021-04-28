/**
 *  Heat2Encoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.protocol;

import java.nio.BufferOverflowException;
import java.util.TreeMap;
import java.util.Vector;

import ea.blaze.tdf.BaseType;
import ea.blaze.tdf.Bitfield;
import ea.blaze.tdf.BlazeObjectId;
import ea.blaze.tdf.BlazeObjectType;
import ea.blaze.tdf.Blob;
import ea.blaze.tdf.MapHelper;
import ea.blaze.tdf.Tdf;
import ea.blaze.tdf.TimeValue;
import ea.blaze.tdf.Union;
import ea.blaze.tdf.VariableTdfContainer;
import ea.blaze.tdf.VectorHelper;
import ea.blaze.tdf.types.TdfBoolean;
import ea.blaze.tdf.types.TdfChar;
import ea.blaze.tdf.types.TdfByte;
import ea.blaze.tdf.types.TdfEnum;
import ea.blaze.tdf.types.TdfFloat;
import ea.blaze.tdf.types.TdfInteger;
import ea.blaze.tdf.types.TdfLong;
import ea.blaze.tdf.types.TdfShort;
import ea.blaze.tdf.types.TdfString;
import ea.blaze.tdf.types.TdfUInt64;

/**
 * Encodes provided TDFs into byte arrays on output stream using the HEAT2 protocol.
 */
public class Heat2Encoder extends TdfEncoder {

	/** Whether to encode a header for the current type. */
	private boolean mEncodeHeader = true;
	
	/** The error count. */
	private int mErrorCount = 0;
	
	/** The temp byte buf for efficient reuse without reallocating often. */
	private byte mBuf[] = new byte[Heat2Util.VARSIZE_MAX_ENCODE_SIZE];
	
	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Encoder#getName()
	 */
	@Override
	public String getName() {
		return "heat";
	}

	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Encoder#getType()
	 */
	@Override
	public Type getType() {
		return Type.HEAT2;
	}
	
	/**
	 * Returns the encoded size of the TDF.
	 * 
	 * @return the int
	 */
	public int encodeSize() {
		if(mByteBuffer != null) {
			return mByteBuffer.position();
		}
		else {
			return 0;
		}
	}
	
	/**
	 * Encode header.
	 * 
	 * @param tag
	 *            the tag
	 * @param type
	 *            the type
	 * @return true, if successful
	 */
	private boolean encodeHeader(long tag, BaseType type) {

		//System.out.println("Writing header with tag [" + String.format("%8x", (int)tag) + "] with type [" + type.ordinal() + "] to offset " + mByteBuffer.position() + ".");

		mBuf[0] = (byte)(((int)tag) >> 24);
		mBuf[1] = (byte)(((int)tag) >> 16);
		mBuf[2] = (byte)(((int)tag) >> 8);
		mBuf[Heat2Util.HEADER_TYPE_OFFSET] = (byte)(type.ordinal() & Heat2Util.ELEMENT_TYPE_MAX);	
	   
		try {
			mByteBuffer.put(mBuf, 0, Heat2Util.HEADER_SIZE);
			return true;
		}
		catch(BufferOverflowException ex) {
			System.err.println("Failed to write varsize integer: " + ex.getMessage());
			mErrorCount++;
			return false;
		}
	}
	
	/**
	 * Encode varsize integer.
	 * 
	 * @param value
	 *            the value
	 * @return true, if successful
	 */
	private boolean encodeVarsizeInteger(long value) {

		try {
		    if (value == 0) {
		    	mBuf[0] = 0;
		        mByteBuffer.put(mBuf, 0, 1);
		        return true;
		    }
		    
		    int oidx = 0;
		    
		    if (value < 0) {
		        value = -value;
	
		        // encode as a negative value
		        mBuf[oidx++] = (byte)(value | (Heat2Util.VARSIZE_MORE | Heat2Util.VARSIZE_NEGATIVE));
		    }
		    else {
		    	mBuf[oidx++] = (byte)((value & (Heat2Util.VARSIZE_NEGATIVE - 1)) | Heat2Util.VARSIZE_MORE);
		    }
		    
		    value >>= 6;
	
		    while (value > 0) {
		    	mBuf[oidx++] = (byte)(value | Heat2Util.VARSIZE_MORE);
		        value >>= 7;
		    }
		    
		    mBuf[oidx-1] &= ~Heat2Util.VARSIZE_MORE;
	
		    
		    mByteBuffer.put(mBuf, 0, oidx);
		    return true;
		}
		catch(BufferOverflowException ex) {
			System.err.println("Failed to write varsize integer: " + ex.getMessage());
			mErrorCount++;
			return false;
		}
	}
	
	/**
	 * Encode header and varsize integer.
	 * 
	 * @param tag
	 *            the tag
	 * @param value
	 *            the value
	 * @return true, if successful
	 */
	private boolean encodeHeaderAndVarsizeInteger(long tag, long value) {
		if(mEncodeHeader) {
			if(!encodeHeader(tag, BaseType.TDF_TYPE_INTEGER)) {
				return false;
			}
		}
		
		return encodeVarsizeInteger(value);
	}
	
	/**
	 * Encode type.
	 * 
	 * @param type
	 *            the type
	 * @return true, if successful
	 */
	private boolean encodeType(BaseType type) {
		try {
			mByteBuffer.put((byte)(type.ordinal()));
			return true;
		}
		catch(BufferOverflowException ex) {
			System.err.println("Failed to encode base type: " + ex.getMessage());
			mErrorCount++;
			return false;
		}
	}
	
	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf tdf, Tdf referenceValue) {
		tdf.visit(this, tdf, tdf);
		return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Union tdf, Union referenceValue) {
		//We have to write the active member with tag 0, so just drop to the normal union encode.
		visit(tdf, tdf, 0, tdf, referenceValue);
		return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.Vector, java.util.Vector, ea.blaze.tdf.VectorHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value,
			Vector<?> referenceValue, VectorHelper vectorHelper) {
		if(value.size() == 0) {
			return;
		}
		
		if(mByteBuffer == null) {
			mErrorCount++;
			return;
		}
		
		if(mEncodeHeader) {
			if(!encodeHeader(tag, BaseType.TDF_TYPE_LIST)) {
				return;
			}
		}
		
		if(encodeType(vectorHelper.getValueType()) &&
		   encodeVarsizeInteger(value.size())) {
			boolean tmpEncodeHeader = mEncodeHeader;
			mEncodeHeader = false;
			vectorHelper.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
			mEncodeHeader = tmpEncodeHeader;
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.TreeMap, java.util.TreeMap, ea.blaze.tdf.MapHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag,
			TreeMap<?, ?> value, TreeMap<?, ?> referenceValue,
			MapHelper mapHelper) {
		if(value.size() == 0) {
			return;
		}
		
		if(mByteBuffer == null) {
			mErrorCount++;
			return;
		}
		
		if(mEncodeHeader) {
			if(!encodeHeader(tag, BaseType.TDF_TYPE_MAP)) {
				return;
			}
		}
		
		if(encodeType(mapHelper.getKeyType()) &&
		   encodeType(mapHelper.getValueType()) &&
		   encodeVarsizeInteger(value.size())) {
			boolean tmpEncodeHeader = mEncodeHeader;
			mEncodeHeader = false;
			mapHelper.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
			mEncodeHeader = tmpEncodeHeader;
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfBoolean, ea.blaze.tdf.types.TdfBoolean, boolean)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfBoolean value,
			TdfBoolean referenceValue, boolean defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, (value.get() ? 1 : 0));
	}
    
    /* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfChar, ea.blaze.tdf.types.TdfChar, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfChar value,
			TdfChar referenceValue, char defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, (int)value.get().charValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfByte, ea.blaze.tdf.types.TdfByte, byte)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfByte value,
			TdfByte referenceValue, byte defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, value.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfShort, ea.blaze.tdf.types.TdfShort, short)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfShort value,
			TdfShort referenceValue, short defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, value.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, int)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, int defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, value.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfLong, ea.blaze.tdf.types.TdfLong, long)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfLong value,
			TdfLong referenceValue, long defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, value.get());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfUInt64 value,
			TdfUInt64 referenceValue, TdfUInt64 defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, value.get().longValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfFloat, ea.blaze.tdf.types.TdfFloat, float)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfFloat value,
			TdfFloat referenceValue, float defaultValue) {
		if(!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_FLOAT)) {
			if(mByteBuffer != null) {
				int tmpValue = Float.floatToIntBits(value.floatValue());
				mBuf[0] = (byte)( tmpValue >> 24);
				mBuf[1] = (byte)( tmpValue >> 16);
				mBuf[2] = (byte)( tmpValue >> 8);
				mBuf[3] = (byte) tmpValue;
				try {
					mByteBuffer.put(mBuf, 0, Heat2Util.FLOAT_SIZE);
				}
				catch(BufferOverflowException ex) {
					System.err.println("Error writing float to buffer: " + ex.getMessage());
					mErrorCount++;
				}
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Bitfield, ea.blaze.tdf.Bitfield)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Bitfield value,
			Bitfield referenceValue) {
		encodeHeaderAndVarsizeInteger(tag, value.getBits());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfString, ea.blaze.tdf.types.TdfString, java.lang.String, long)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfString value,
			TdfString referenceValue, String defaultValue, long maxLength) {

		long length = (long)value.get().length() + 1;
		
		if((!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_STRING)) &&
		   encodeVarsizeInteger(length)) {
			try {
				mByteBuffer.put(value.get().getBytes());
				mByteBuffer.put((byte)0);
			}
			catch(BufferOverflowException ex) {
				System.err.println("Error writing string data: " + ex.getMessage());
				mErrorCount++;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Blob, ea.blaze.tdf.Blob)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Blob value,
			Blob referenceValue) {
		long length = value.getSize();
		
		if((!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_BINARY)) &&
		   encodeVarsizeInteger(length)) {
			try {
				mByteBuffer.put(value.getData());
			}
			catch(BufferOverflowException ex) {
				System.err.println("Error writing blob data: " + ex.getMessage());
				mErrorCount++;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, java.lang.Class, java.lang.Enum)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, TdfEnum enumHelper,
			TdfEnum defaultValue) {
		// use standard integer visitor
		visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue.getValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Tdf value,
			Tdf referenceValue) {
		if(mByteBuffer == null) {
			return (mErrorCount == 0);
		}
		
		if(!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_STRUCT)) {
			boolean tmpEncodeHeader = mEncodeHeader;
			mEncodeHeader = true;
			value.visit(this, rootTdf, value);
			mEncodeHeader = tmpEncodeHeader;
	
			try {
				mByteBuffer.put(Heat2Util.ID_TERM);
			}
			catch(BufferOverflowException ex) {
				System.err.println("IO error writing TDF term byte: " + ex.getMessage());
				mErrorCount++;
			}
		}
		
		return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Union value,
			Union referenceValue) {
		if(mByteBuffer == null) {
			return (mErrorCount == 0);
		}
		
		if(!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_UNION)) {
			
			try {
				mByteBuffer.put((byte)value.getActiveMember());
			}
			catch(BufferOverflowException ex) {
				System.err.println("IO error writing union active member: " + ex.getMessage());
				mErrorCount++;
			}
			
			value.visit(this, rootTdf, value);
		}
		
		return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.VariableTdfContainer, ea.blaze.tdf.VariableTdfContainer)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag,
			VariableTdfContainer value, VariableTdfContainer referenceValue) {
		if(mByteBuffer == null) {
			return (mErrorCount == 0);
		}
		
		if(!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_VARIABLE)) {
			try {
				mByteBuffer.put((byte) (value.isValid() ? 1 : 0));
			}
			catch(BufferOverflowException ex) {
				System.err.println("IO error writing variable tdf present bit: " + ex.getMessage());
				mErrorCount++;
				return false;
			}
			
			if(value.isValid()) {
				if(!value.get().isRegisteredTdf()) {
					System.err.println("Failure: Attempting to encode unregistered TDF as a variable TDF.");
					mErrorCount++;
					return false;
				}
				
				if(encodeVarsizeInteger(value.get().getTdfId())) {
					visit(rootTdf, parentTdf, tag, value.get(), value.get());
				}
				
		        // Place a struct terminator at the end of the encoding to allow for easy skipping on
		        // the decode side.
				try {
					mByteBuffer.put(Heat2Util.ID_TERM);
				}
				catch(BufferOverflowException ex) {
					System.err.println("IO error writing TDF term byte: " + ex.getMessage());
					mErrorCount++;
				}
				
			}
		}
		return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectType, ea.blaze.tdf.BlazeObjectType)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag,
			BlazeObjectType value, BlazeObjectType referenceValue) {
		if(mByteBuffer == null) {
			mErrorCount++;
			return;
		}

		if(!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_BLAZE_OBJECT_TYPE)) {
			if(encodeVarsizeInteger((long) value.getComponentId())) {
				encodeVarsizeInteger((long) value.getTypeId());
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectId, ea.blaze.tdf.BlazeObjectId)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag,
			BlazeObjectId value, BlazeObjectId referenceValue) {
		if(mByteBuffer == null) {
			mErrorCount++;
			return;
		}

		if(!mEncodeHeader || encodeHeader(tag, BaseType.TDF_TYPE_BLAZE_OBJECT_ID)) {
			if(encodeVarsizeInteger((long) value.getType().getComponentId()) &&
			   encodeVarsizeInteger((long) value.getType().getTypeId())) {
				encodeVarsizeInteger(value.getEntityId());
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TimeValue value,
			TimeValue referenceValue, TimeValue defaultValue) {
		encodeHeaderAndVarsizeInteger(tag, value.getMicroSeconds());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.protocol.TdfEncoder#reset()
	 */
	@Override
	protected void reset() {
		mErrorCount = 0;
		mEncodeHeader = true;
	}

}
