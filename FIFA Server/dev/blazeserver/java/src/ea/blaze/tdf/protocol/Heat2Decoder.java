/**
 *  Heat2Decoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.protocol;

import java.io.IOException;
import java.math.BigInteger;
import java.nio.BufferUnderflowException;
import java.util.TreeMap;
import java.util.Vector;

import ea.blaze.tdf.BaseType;
import ea.blaze.tdf.Bitfield;
import ea.blaze.tdf.BlazeObjectId;
import ea.blaze.tdf.BlazeObjectType;
import ea.blaze.tdf.Blob;
import ea.blaze.tdf.MapHelper;
import ea.blaze.tdf.TagInfo;
import ea.blaze.tdf.Tdf;
import ea.blaze.tdf.TdfFactory;
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
 * Decodes input streams containing TDF information encoded using the HEAT2 protocol.
 */
public class Heat2Decoder extends TdfDecoder {

	/**
	 * Internal classed used to wrap BigInteger values needed to decode unsigned 64-bit ints.
	 */
	private static class BigIntegerRef {
		
		/** The big int. */
		private BigInteger mBigInt;
		
		/**
		 * Instantiates a new big integer ref.
		 */
		public BigIntegerRef() {
			mBigInt = BigInteger.ZERO;
		}
		
		/**
		 * Gets the big integer
		 * 
		 * @return the big integer
		 */
		public BigInteger get() { 
			return mBigInt;
		}
		
		/**
		 * Sets the big integer to value specified
		 * 
		 * @param rhs
		 *            the value to be set
		 */
		public void set(BigInteger rhs) {
			mBigInt = rhs;
		}
	}

	// Some static shared temporary static members to allow reading from buffer
	// without having to create new objects each time 
	
	/** A temp big integer value for efficient reuse when decoding varsize integers. */
	private BigIntegerRef mTempValue = new BigIntegerRef();
	
	/** The temp byte buf for efficient reuse when reading byte information from input stream. */
	private byte mBuf[] = new byte[Heat2Util.HEADER_SIZE];
	
	/** Whether to decode a header for next value. */
	private boolean mDecodeHeader = true;
	
	/** The error count of the current decoding. */
	private int mErrorCount = 0;

	/**
	 * Instantiates a new heat2 decoder.
	 * 
	 * @param validateStringLength
	 *            whether to validate string length during decoding.
	 * @param validateEnums
	 *            whether to validate enums during decoding.
	 */
	public Heat2Decoder(boolean validateStringLength, boolean validateEnums) {
		super(validateStringLength, validateEnums);
	}
	
	/**
	 * Read next header from mByteBuffer input stream.
	 * 
	 * @param tag
	 *            the tag we should see in the next header
	 * @param type
	 *            the type we should see in the next header
	 * @return true, if the header contained the specified tag and type.  False otherwise.
	 */
	public boolean readHeader(long tag, BaseType type) {
		
		try
		{
			if(mByteBuffer == null || mByteBuffer.hasRemaining() == false) {
				return false;
			}
			
			while(mByteBuffer.remaining() >= Heat2Util.HEADER_SIZE) {
				mByteBuffer.mark();		
				mByteBuffer.get(mBuf, 0, Heat2Util.HEADER_SIZE);
				
				// We've reached the end of the TDF if we see this, 
				// reset our location and return false
				if(mBuf[0] == Heat2Util.ID_TERM) {
					mByteBuffer.reset();
					return false;
				}
				
				// All bytes are stored are unsigned, so we have to promote them to shorts
				long bufTag = (long) ((Heat2Util.promoteUnsignedByte(mBuf[0]) << 24) | (Heat2Util.promoteUnsignedByte(mBuf[1]) << 16) | (Heat2Util.promoteUnsignedByte(mBuf[2]) << 8));
				int bufType = Heat2Util.promoteUnsignedByte(mBuf[Heat2Util.HEADER_TYPE_OFFSET]);

				if(bufType >= BaseType.TDF_TYPE_MAX.ordinal()) {
					// ERROR: Invalid type or the reserved bits are not all 0.
					mErrorCount++;
					return false;
				}
				
				if(bufTag == tag) {	
					// This is the tag we are looking for
					
					if(bufType != type.ordinal()) {
						// ERROR: Matching tag but mismatched type
						mErrorCount++;
						return false;
					}
					
					return true;
				}
				
				if(bufTag > tag) {
					// Tag doesn't exist.  This is not an error condition as it just means that the
		            // encoded stream didn't have this element and we'll use the default instead.
		            // Push the header back on the stream since we don't actually want to consume it here.
					mByteBuffer.reset();
					return false;
				}
				
				// Unrecognized tag in the buffer so skip it
				if(!skipElement(BaseType.values()[bufType])) {
					mErrorCount++;
					return false;
				}
			}
		}
		catch(BufferUnderflowException ex) {
			mErrorCount++;
			// Log exception
		}
		
		return false;
	}
	
	/**
	 * Reads the mByteBuffer input stream until a struct terminator is reached.
	 * 
	 * @return true if a struct terminator was found.
	 */
	private boolean getStructTerminator()
	{
	    if (mByteBuffer == null)
	        return false;
	    
	    try {
		    while (mByteBuffer.remaining() >= 1) {
		    	mByteBuffer.mark();
		    	
		    	mByteBuffer.get(mBuf, 0, Heat2Util.HEADER_SIZE);
		        
		        if (mBuf[0] == Heat2Util.ID_TERM) {
		        	mByteBuffer.reset();
		            mByteBuffer.position(mByteBuffer.position() + 1);
		            return true;
		        }

		        int bufType = Heat2Util.promoteUnsignedByte(mBuf[Heat2Util.HEADER_TYPE_OFFSET]);
		        
		        if(BaseType.valid(bufType)) {
			        if (!skipElement(BaseType.values()[bufType]))
			            break;
		        }
		        else {
		        	mErrorCount++;
		        	break;
		        }
		    }
	    }
	    catch(BufferUnderflowException ex) {
	    	mErrorCount++;
	    }
	    return false;
	}
	
	/**
	 * Reads past the specified type in the mByteBuffer input stream.
	 * 
	 * @param bufType
	 *            the type to read past in the input stream.
	 * @return true, if type was successfully skipped.
	 * 
	 * @throws IOException
	 *             Signals that an I/O exception has occurred.
	 */
	private boolean skipElement(BaseType bufType) {
	    if (mByteBuffer == null)
	        return false;

	    boolean rc = true;
	    
	    switch (bufType)
	    {
	        case TDF_TYPE_INTEGER:
	        {
	            if (!decodeVarsizeInteger(mTempValue))
	                rc = false;
	            break;
	        }

	        case TDF_TYPE_STRING:
	        case TDF_TYPE_BINARY:
	        {
	            if (!decodeVarsizeInteger(mTempValue) || (mTempValue.get().longValue() < 0)
	                    || (mByteBuffer.remaining() < mTempValue.get().longValue())) {
	                rc = false;
	            }
	            else {
	            	mByteBuffer.position((int)(mByteBuffer.position() + mTempValue.get().longValue()));
	            }
	            break;
	        }

	        case TDF_TYPE_STRUCT:
	        {
	            rc = getStructTerminator();
	            break;
	        }

	        case TDF_TYPE_LIST:
	        {
	            if (mByteBuffer.remaining() < 1) {
	                rc = false;
	            }
	            else {
	            	mByteBuffer.get(mBuf, 0, 1);
	                int listType = Heat2Util.promoteUnsignedByte(mBuf[0]);

	                if(!BaseType.valid(listType)) {
	                	rc = false;
	                }
	                else {
		                if (!decodeVarsizeInteger(mTempValue) || (mTempValue.get().intValue() < 0)) {
		                    rc = false;
		                }
		                else {
		                	int listMemberCount = mTempValue.get().intValue();
		                    for(int idx = 0; (idx < listMemberCount) && rc; ++idx)
		                        rc = skipElement(BaseType.values()[listType]);
		                }
	                }
	            }
	            break;
	        }

	        case TDF_TYPE_MAP:
	        {
	            if (mByteBuffer.remaining() < 2) {
	                rc = false;
	            }
	            else {
	            	mByteBuffer.get(mBuf, 0, 2);
	                int keyType = Heat2Util.promoteUnsignedByte(mBuf[0]);
	                int valueType = Heat2Util.promoteUnsignedByte(mBuf[1]);

	                if(BaseType.valid(keyType) && BaseType.valid(valueType)) {
		                if (!decodeVarsizeInteger(mTempValue) || (mTempValue.get().intValue() < 0)) {
		                    rc = false;
		                }
		                else {
		                	int mapLength = mTempValue.get().intValue();
		                    for(int idx = 0; (idx < mapLength) && rc; ++idx) {
		                        rc = skipElement(BaseType.values()[keyType]);
		                        if (rc)
		                            rc = skipElement(BaseType.values()[valueType]);
		                    }
		                }
	                }
	                else {
	                	rc = false;
	                }
	            }
	            break;
	        }

	        case TDF_TYPE_UNION:
	        {
	            if (mByteBuffer.remaining() < 1)  {
	                rc = false;
	            }
	            else {
	                // Grab the active member index
	            	mByteBuffer.get(mBuf, 0, 1);
	                int activeMember = Heat2Util.promoteUnsignedByte(mBuf[0]);
	                
	                if (activeMember != Union.INVALID_MEMBER_INDEX) {
	                    if (mByteBuffer.remaining() < Heat2Util.HEADER_SIZE) {
	                        rc = false;
	                    }
	                    else {
	                        // Skip the union member
	                    	mByteBuffer.get(mBuf, 0, Heat2Util.HEADER_SIZE);
	                        int unionType = Heat2Util.promoteUnsignedByte(mBuf[Heat2Util.HEADER_TYPE_OFFSET]);
	                        
	                        if(BaseType.valid(unionType)) {
	                        	rc = skipElement(BaseType.values()[unionType]);
	                        }
	                        else {
	                        	rc = false;
	                        }
	                    }
	                }
	            }
	            break;
	        }

	        case TDF_TYPE_VARIABLE:
	        {
	            if (mByteBuffer.remaining() < 1) {
	                rc = false;
	            }
	            else {
	                // variable TDFs are terminated like a structure if present so just skip to the
	                // next terminator.
	            	mByteBuffer.get(mBuf, 0, 1);
	                long present = mBuf[0];
	                if (present != 0) {
	                    // Pull the TDF ID
	                    rc = decodeVarsizeInteger(mTempValue);
	                    if (rc) {
	                        // Skip the TDF itself
	                        rc = getStructTerminator();
	                    }
	                }
	            }
	            break;
	        }

	        case TDF_TYPE_MAX:
	        default:
	            rc = false;
	            break;
	    }
	    if (!rc) {
	        mErrorCount++;
	    }
	    return rc;
	}

	/**
	 * Decode next varsize integer in the mByteBuffer input stream.
	 * 
	 * @param value
	 *            wrapper of value to be set, if read successfully.
	 * @return true, if varsize integer was read successfully and the value was set.
	 */
	public boolean decodeVarsizeInteger(BigIntegerRef value) {
		try {
			long len = mByteBuffer.remaining();
			
			if(len == 0) {
				return false;
			}
			
			mByteBuffer.mark();
			
			byte buf[] = new byte[1];
			
			mByteBuffer.get(buf, 0, 1);
		
			// First byte has negative bit
			boolean valueIsNegative = (buf[0] & Heat2Util.VARSIZE_NEGATIVE) == Heat2Util.VARSIZE_NEGATIVE;
			boolean hasMore = (buf[0] & Heat2Util.VARSIZE_MORE) == Heat2Util.VARSIZE_MORE;
			
			BigInteger v = BigInteger.valueOf((long)(buf[0] & (Heat2Util.VARSIZE_NEGATIVE-1)));
			
		    if (hasMore) {
		    	int shift = 6;
			    hasMore = false;
			    BigInteger bufVal;
			    
			    while(mByteBuffer.remaining() > 0) {
			    	mByteBuffer.get(buf, 0, 1);
			    
			    	bufVal = BigInteger.valueOf((long)buf[0]);
			    	bufVal = bufVal.and(BigInteger.valueOf((long)(Heat2Util.VARSIZE_MORE - 1)));
			    	bufVal = bufVal.shiftLeft(shift);
			    	v = v.or(bufVal);
			    	
		            hasMore = ((buf[0] & Heat2Util.VARSIZE_MORE) != 0);
		          
		            if (!hasMore) {
		                break;
		            }
		            shift += 7;
		        }
			    
		        if (hasMore) {
		            // We finished decoding with a MORE byte so the encoding is incomplete.
			        mErrorCount++;
		            value.set(BigInteger.ZERO);
		            return false;
		        }
		    }
	
		    
		    if (valueIsNegative) {
		        value.set((v != BigInteger.ZERO) ? v.negate() : BigInteger.valueOf(Long.MIN_VALUE));
		    }
		    else {
		        value.set(v);
		    }
		    
		    return true;
		}
		catch(BufferUnderflowException ex) {
			mErrorCount++;
			// log exception?
			return false;
		}
	}
	
	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Decoder#getName()
	 */
	@Override
	public String getName() {
		return "heat";
	}

	/* (non-Javadoc)
	 * @see ea.blaze.protocol.Decoder#getType()
	 */
	@Override
	public Type getType() {
		return Type.HEAT2;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf tdf, Tdf referenceValue) {
		tdf.visit(this, tdf, referenceValue);
		return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Union tdf, Union referenceValue) {
		tdf.visit(this, tdf, referenceValue);
		return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.Vector, java.util.Vector, ea.blaze.tdf.VectorHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, Vector<?> value,
			Vector<?> referenceValue, VectorHelper memberHelper) {
		if(mByteBuffer == null) {
			return;
		}
		
		if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_LIST)) {
			try {
				if(mByteBuffer.remaining() < 1) {
					mErrorCount++;
					return;
				}
				
				mByteBuffer.mark();
				int listType = Heat2Util.promoteUnsignedByte((byte)mByteBuffer.get());
				
				if(listType != memberHelper.getValueType().ordinal()) {
					// Vector element type mismatch, back up and skip over
					// the entire list.
					mByteBuffer.reset();
					skipElement(BaseType.TDF_TYPE_LIST);
					memberHelper.initializeVector(value, 0);
					return;
				}
				
				if(!decodeVarsizeInteger(mTempValue)) {
					// Couldn't decode length of vector
					mErrorCount++;
					return;
				}
				
				boolean tmpDecodeHeader = mDecodeHeader;
				mDecodeHeader = false;
				memberHelper.initializeVector(value, mTempValue.get().intValue());
				memberHelper.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
				mDecodeHeader = tmpDecodeHeader;
			}
			catch(BufferUnderflowException ex) {
				mErrorCount++;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, java.util.TreeMap, java.util.TreeMap, ea.blaze.tdf.MapHelper)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag,
			TreeMap<?, ?> value, TreeMap<?, ?> referenceValue,
			MapHelper mapHelper) {
		if(mByteBuffer == null) {
			return;
		}
		
		if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_MAP)) {
			try {
				if(mByteBuffer.remaining() < 2) {
					mErrorCount++;
					return;
				}
				
				mByteBuffer.mark();
				int keyType = Heat2Util.promoteUnsignedByte((byte)mByteBuffer.get());
				int valueType = Heat2Util.promoteUnsignedByte((byte)mByteBuffer.get());
				
				if(keyType != mapHelper.getKeyType().ordinal() || 
				   valueType != mapHelper.getValueType().ordinal()) {
					// Map type mismatch, back up and skip over
					// the entire thing.
					mByteBuffer.reset();
					skipElement(BaseType.TDF_TYPE_MAP);
					mapHelper.initializeMap(value, 0);
					return;
				}
				
				if(!decodeVarsizeInteger(mTempValue)) {
					// Couldn't decode length of map
					mErrorCount++;
					return;
				}
				
				boolean tmpDecodeHeader = mDecodeHeader;
				mDecodeHeader = false;
				mapHelper.initializeMap(value, mTempValue.get().intValue());
				mapHelper.visitMembers(this, rootTdf, parentTdf, tag, value, referenceValue);
				mDecodeHeader = tmpDecodeHeader;
			}
			catch(BufferUnderflowException ex) {
				mErrorCount++;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfBoolean, ea.blaze.tdf.types.TdfBoolean, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfBoolean value,
			TdfBoolean referenceValue, boolean defaultValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, defaultValue ? BigInteger.ONE : BigInteger.ZERO);
		value.set(mTempValue.get().compareTo(BigInteger.ZERO) != 0);
	}
    
    /* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfChar, ea.blaze.tdf.types.TdfChar, boolean)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfChar value,
			TdfChar referenceValue, char defaultValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, BigInteger.valueOf((int)defaultValue));
		value.set((char)mTempValue.get().intValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfByte, ea.blaze.tdf.types.TdfByte, byte)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfByte value,
			TdfByte referenceValue, byte defaultValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, BigInteger.valueOf(defaultValue));
		value.set(mTempValue.get().byteValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfShort, ea.blaze.tdf.types.TdfShort, short)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfShort value,
			TdfShort referenceValue, short defaultValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, BigInteger.valueOf(defaultValue));
		value.set(mTempValue.get().shortValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, int)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, int defaultValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, BigInteger.valueOf(defaultValue));
		value.set(mTempValue.get().intValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfLong, ea.blaze.tdf.types.TdfLong, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfLong value,
			TdfLong referenceValue, long defaultValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, BigInteger.valueOf(defaultValue));
		value.set(mTempValue.get().longValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64, ea.blaze.tdf.types.TdfUInt64)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfUInt64 value,
			TdfUInt64 referenceValue, TdfUInt64 defaultValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, defaultValue.get());
		value.set(mTempValue.get().longValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfFloat, ea.blaze.tdf.types.TdfFloat, float)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfFloat value,
			TdfFloat referenceValue, float defaultValue) {
		
		if(mDecodeHeader) {
            if (!readHeader(tag, BaseType.TDF_TYPE_FLOAT)) {
                return;
            }
		}
		
		try {
			if(mByteBuffer.remaining() < Heat2Util.FLOAT_SIZE) {
				mErrorCount++;
				value.set(defaultValue);
			}
			
			mByteBuffer.get(mBuf, 0, Heat2Util.FLOAT_SIZE);
			int intValue = 0;
			intValue |= (((int)mBuf[0] & 0xff) << 24);
			intValue |= (((int)mBuf[1] & 0xff) << 16);
			intValue |= (((int)mBuf[2] & 0xff) << 8);
			intValue |= ((int)mBuf[3] & 0xff);
			
			value.set(Float.intBitsToFloat(intValue));
		}
		catch(BufferUnderflowException ex) {
			mErrorCount++;
			value.set(defaultValue);
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Bitfield, ea.blaze.tdf.Bitfield)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Bitfield value,
			Bitfield referenceValue) {
		decodeNextHeaderAndInteger(tag, mTempValue, BigInteger.ZERO);
		value.setBits(mTempValue.get().longValue());
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfString, ea.blaze.tdf.types.TdfString, java.lang.String, long)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TdfString value,
			TdfString referenceValue, String defaultValue, long maxLength) {
		if(mByteBuffer == null) {
			return;
		}
		
		if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_STRING)) {
			int stringLen;
			// Get string length
			if(!decodeVarsizeInteger(mTempValue)) {
				// Couldn't decode valid int.
				return;
			}
			stringLen = mTempValue.get().intValue();
			
			if(stringLen < 0) {
				// Can't have negative length;
				mErrorCount++;
				return;
			}
			
			if(mValidateStringLength && maxLength > 0 && maxLength < stringLen) {
				System.err.println("Error decoding string for field " + TagInfo.decodeTag(tag, false) +
						" in " + parentTdf.getClass().getName() + " as incoming string length (" + stringLen + ") " + 
						" exceeds max length (" + maxLength + ")!");
				mErrorCount++;
				mValidationError = ValidationError.STRING_TOO_LONG;
			}
			
			try {
				if(mByteBuffer.remaining() < stringLen) {
					// We don't have enough bytes in buffer for string of this length
					mErrorCount++;
					return;
				}
				
				byte stringBytes[] = new byte[stringLen];
				mByteBuffer.get(stringBytes, 0, stringLen);
				value.set(new String(stringBytes, 0, stringLen-1));
			}
			catch(BufferUnderflowException ex) {
				mErrorCount++;
				return;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Blob, ea.blaze.tdf.Blob)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, Blob value,
			Blob referenceValue) {
		if(mByteBuffer == null) {
			return;
		}
		
		if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_BINARY)) {
			int dataLen;
			// Get string length
			if(decodeVarsizeInteger(mTempValue)) {
				return;
			}
			dataLen = mTempValue.get().intValue();
			
			if(dataLen < 0) {
				// Can't have negative length;
				mErrorCount++;
				return;
			}
			
			try {
				if(mByteBuffer.remaining() < dataLen) {
					// We don't have enough bytes in buffer for blob of this length
					mErrorCount++;
					return;
				}
				
				byte dataBytes[] = new byte[dataLen];
				mByteBuffer.get(dataBytes, 0, dataLen);
				value.setData(dataBytes);
			}
			catch(BufferUnderflowException ex) {
				mErrorCount++;
				return;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.types.TdfInteger, ea.blaze.tdf.types.TdfInteger, java.lang.Class, java.lang.Enum)
	 */
	@Override
	public void visit(Tdf rootTdf, Tdf parentTdf, long tag, TdfInteger value,
			TdfInteger referenceValue, TdfEnum helper,
			TdfEnum defaultValue) {
		// Decode the integer based enum value using integer visit method
		visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue.getValue());
		
		if(mValidateEnums) {
			TdfEnum enumValue = helper.lookupValue(value.get());
			if(enumValue == null) {
				mErrorCount++;
				mValidationError = ValidationError.INVALID_ENUM_VALUE;
			}
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Tdf value,
			Tdf referenceValue) {
	    if (!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_STRUCT)) {
	        boolean tmpDecodeHeader = mDecodeHeader;
	        mDecodeHeader = true;
	        value.visit(this, rootTdf, value);
	        mDecodeHeader = tmpDecodeHeader;
	        getStructTerminator();
	    }
	    return (mErrorCount == 0);
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.Union, ea.blaze.tdf.Union)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag, Union value,
			Union referenceValue) {
		if(mByteBuffer != null) {
			if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_UNION)) {
				try {
					int activeMember = mByteBuffer.get();
					value.switchActiveMember(activeMember);
					
					if(activeMember != Union.INVALID_MEMBER_INDEX) {
						
						if(value.getActiveMember() != Union.INVALID_MEMBER_INDEX) {
							value.visit(this, rootTdf, value);
						}
						else {
			                // Failed to decode the member because the encoded member wasn't recognized by
			                // this decode likely due to mismatched TDF versions between the encode and decode
			                // sides.  Just try and skip the element and move on.
			                if (mByteBuffer.remaining() < Heat2Util.HEADER_SIZE) {
			                    mErrorCount++;
			                }
			                else {
			                    
			                    mByteBuffer.position(mByteBuffer.position() + Heat2Util.HEADER_TYPE_OFFSET);		                    
			                    int skipType = mByteBuffer.get();
			                    if(BaseType.valid(skipType)) {
			                    	skipElement(BaseType.values()[skipType]);
			                    }
			                    else {
			                    	mErrorCount++;
			                    }
			                }
						}
					}
				}
				catch(BufferUnderflowException ex) {
					mErrorCount++;
				}
			}
			
		}
		
		return mErrorCount == 0;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.VariableTdfContainer, ea.blaze.tdf.VariableTdfContainer)
	 */
	@Override
	public boolean visit(Tdf rootTdf, Tdf parentTdf, long tag,
			VariableTdfContainer value, VariableTdfContainer referenceValue) {
		if(mByteBuffer != null) {
		
			if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_VARIABLE)) {
				try {
					if(mByteBuffer.remaining() == 0) {
						mErrorCount++;
					}
					else {
		            	mByteBuffer.get(mBuf, 0, 1);
		                long present = mBuf[0];
		                if (present != 0) {
		                	// The TDF is present so create and decode it
		                	long tdfId;
		                	
		                	if(decodeVarsizeInteger(mTempValue) &&
		                		(tdfId = mTempValue.get().longValue()) != Tdf.INVALID_TDF_ID)
	                		{
		                		Tdf variableTdf = TdfFactory.get().createTdf(tdfId);
		                		
		                		if(variableTdf == null) {
		                			// unrecognized or unregistered variable tdf
		                			mErrorCount++;
		                			return false;
		                		}
		                		else {
		                			visit(rootTdf, parentTdf, tag, variableTdf, variableTdf);
		                			value.set(variableTdf);
		                			getStructTerminator();
		                		}
	                		}
		                	else {
		                		mErrorCount++;
		                	}
		                }
					}
				}
				catch(BufferUnderflowException ex) {
					mErrorCount++;
				}
			}
		}
		
		return mErrorCount == 0;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectType, ea.blaze.tdf.BlazeObjectType)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectType value, BlazeObjectType referenceValue) {

		if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_BLAZE_OBJECT_TYPE)) {
			if(decodeVarsizeInteger(mTempValue)) 
				value.setComponentId(Heat2Util.promoteUnsignedShort(mTempValue.get().shortValue()));
			
			if(decodeVarsizeInteger(mTempValue))
				value.setTypeId(Heat2Util.promoteUnsignedShort(mTempValue.get().shortValue()));
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.BlazeObjectId, ea.blaze.tdf.BlazeObjectId)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag,
			BlazeObjectId value, BlazeObjectId referenceValue) {

		if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_BLAZE_OBJECT_ID)) {
			if(decodeVarsizeInteger(mTempValue)) 
				value.getType().setComponentId(Heat2Util.promoteUnsignedShort(mTempValue.get().shortValue()));
			
			if(decodeVarsizeInteger(mTempValue))
				value.getType().setTypeId(Heat2Util.promoteUnsignedShort(mTempValue.get().shortValue()));
			
			if(decodeVarsizeInteger(mTempValue))
				value.setEntityId(mTempValue.get().longValue());
		}
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.Visitor#visit(ea.blaze.tdf.Tdf, ea.blaze.tdf.Tdf, long, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue, ea.blaze.tdf.TimeValue)
	 */
	@Override
	public void visit(Tdf roottdf, Tdf parentTdf, long tag, TimeValue value,
			TimeValue referenceValue, TimeValue defaultValue) {
		
		decodeNextHeaderAndInteger(tag, mTempValue, BigInteger.valueOf(defaultValue.getMicroSeconds()));
        value.set(mTempValue.get().longValue());

	}

	/**
	 * Decodes next header and integer in the input stream.  If no integer was read from the
	 * stream, the value is set to the default value. 
	 * 
	 * @param tag
	 *            the tag
	 * @param value
	 *            the value
	 * @param defaultValue
	 *            the default value
	 */
	public void decodeNextHeaderAndInteger(long tag, BigIntegerRef value, BigInteger defaultValue) {
		value.set(defaultValue);
		
		if(!mDecodeHeader || readHeader(tag, BaseType.TDF_TYPE_INTEGER)) {
            decodeVarsizeInteger(value);
		}
	}

	/**
	 * Peeks ahead and checks if the next header in the input stream matches
	 * the specified tag and type.  buffer remains at current position in stream.
	 * 
	 * @param tag
	 *            the tag to check for
	 * @param type
	 *            the type to check for
	 * @return true, if the next header was available on the stream and contained the expected information.
	 * @throws IOException
	 *             Signals that an I/O exception has occurred.
	 */
	public boolean peekHeader(TdfLong tag, TdfInteger type) throws IOException {
	    if (mByteBuffer == null)
	        return false;

	    mByteBuffer.mark();
	    
	    if(mByteBuffer.remaining() < Heat2Util.HEADER_SIZE) {
	    	return false;
	    }
	    else {
	    	mByteBuffer.get(mBuf, 0, Heat2Util.HEADER_SIZE);
	    	mByteBuffer.reset();
	    	if(mBuf[0] == Heat2Util.ID_TERM) {
	    		return false;
	    	}
	    	
	    	tag.set((long) ((Heat2Util.promoteUnsignedByte(mBuf[0]) << 24) | (Heat2Util.promoteUnsignedByte(mBuf[1]) << 16) | (Heat2Util.promoteUnsignedByte(mBuf[2]) << 8)));
	    	type.set((int)Heat2Util.promoteUnsignedByte(mBuf[Heat2Util.HEADER_TYPE_OFFSET]));
	    	return true;
	    }
	}

	/**
	 * Peeks ahead and checks if the next list type matches the expected type.  Buffer
	 * remains at current position in stream.
	 * 
	 * @param type
	 *            the type to check for
	 * @return true, if list type was successfully read and matched expected list type.
	 * @throws IOException
	 *             Signals that an I/O exception has occurred.
	 */
	public boolean peekListType(TdfInteger type) throws IOException {
	    if (mByteBuffer == null)
	        return false;

	    if(mByteBuffer.remaining() < (Heat2Util.HEADER_SIZE + 1)) {
	    	return false;
	    }
	    
	    mByteBuffer.mark();
	    mByteBuffer.position(mByteBuffer.position() + Heat2Util.HEADER_SIZE);
	    type.set((int)Heat2Util.promoteUnsignedByte((byte)mByteBuffer.get()));
	    mByteBuffer.reset();
	    return true;
	}
	
	/**
	 * Peeks ahead and checks if the next map key and value type matches the 
	 * expected types.  Buffer remains at current position in stream.
	 * 
	 * @param keyType
	 *            the key type to check for
	 * @param valueType
	 *            the value type to check for
	 * @return true, if both types were successfully read and matched expected map types.
	 * @throws IOException
	 *             Signals that an I/O exception has occurred.
	 */
	public boolean peekMapType(TdfInteger keyType, TdfInteger valueType) throws IOException {
	    if (mByteBuffer == null)
	        return false;

	    if(mByteBuffer.remaining() < (Heat2Util.HEADER_SIZE + 2)) {
	    	return false;
	    }
	    
	    mByteBuffer.mark();
	    mByteBuffer.position(mByteBuffer.position() + Heat2Util.HEADER_SIZE);
	    
	    keyType.set((int)Heat2Util.promoteUnsignedByte((byte)mByteBuffer.get()));
	    valueType.set((int)Heat2Util.promoteUnsignedByte((byte)mByteBuffer.get()));
	    mByteBuffer.reset();
	    return true;
	}

	/* (non-Javadoc)
	 * @see ea.blaze.tdf.protocol.TdfDecoder#reset()
	 */
	@Override
	protected void reset() {
		mErrorCount = 0;
		mDecodeHeader = true;
	}
}
