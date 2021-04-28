/**
 *  TdfDecoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.protocol;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;

import ea.blaze.protocol.Decoder;
import ea.blaze.tdf.Tdf;
import ea.blaze.tdf.Visitor;

/**
 * Abstract TdfDecoder class providing base decoder functionality.  Implementing classes
 * must implement Decoder and Visitor interfaces.
 */
public abstract class TdfDecoder implements Decoder, Visitor {
	
	/**
	 * The enum for validation errors during decoding.
	 */
	public enum ValidationError {
		
		NO_ERROR,
		INVALID_ENUM_VALUE,
		STRING_TOO_LONG;		
	}
	
	/** The input buffer. */
	protected ByteBuffer mByteBuffer;
	
	/** The decode result. */
	protected boolean mDecodeResult;
	
	/** Whether this decoder should validate string length. */
	protected boolean mValidateStringLength = true;
	
	/** Whether this decoder should validate enums. */
	protected boolean mValidateEnums = true;
	
	/** The validation error. */
	protected ValidationError mValidationError = ValidationError.NO_ERROR;
	
	/**
	 * Returns InputStream that reads directly from underlying byte buffer.
	 * 
	 * @return InputStream that reads directly from underlying byte buffer.
	 */
	protected InputStream getBufferAsInputStream() {
		return new InputStream() {
	        public synchronized int read() throws IOException {
	            if (!mByteBuffer.hasRemaining()) {
	                return -1;
	            }
	            return (int)(mByteBuffer.get() & 0xFF);
	        }

	        public synchronized int read(byte[] bytes, int off, int len) throws IOException {
	            if (!mByteBuffer.hasRemaining()) {
	                return -1;
	            }
	            // Read only what's left
	            len = Math.min(len, mByteBuffer.remaining());
	            mByteBuffer.get(bytes, off, len);
	            return len;
	        }
		};
	}
	
	/**
	 * Instantiates a new TDF decoder.
	 * 
	 * @param validateStringLength
	 *            Whether this decoder should validate string length
	 * @param validateEnums
	 *            Whether this decoder should validate enums
	 */
	public TdfDecoder(boolean validateStringLength, boolean validateEnums) {
		mValidateStringLength = validateStringLength;
		mValidateEnums = validateEnums;
	}
	
	/**
	 * Sets the input buffer, and resets the decoder to clean state.
	 * 
	 * @param inputStream
	 *            the new buffer
	 */
	public void setBuffer(ByteBuffer buffer) {
		reset();
		this.mByteBuffer = buffer;
	}
	
	/**
	 * Decodes the information stored in the input buffer into the TDF.  It is
	 * up to the caller to provide the TDF class that matches the incoming
	 * data in the input stream.
	 * 
	 * @param buffer
	 *            the byte buffer to be decoded
	 * @param value
	 *            the TDF value to be populated with decoded information
	 */
	public void decode(ByteBuffer buffer, Tdf value) {
		reset();
		this.mByteBuffer = buffer;
		mDecodeResult = visit(value, value);
	}
	
	/**
	 * Reset this decoder to pristine state.  Does not change the marker on the input 
	 * stream.
	 */
	protected abstract void reset();
	
	/**
	 * Gets the validation result of the last decode done by this decoder.
	 * 
	 * @return the validation result
	 */
	public ValidationError validationResult() {
		return mValidationError;
	}
	
	/**
	 * Gets the decode result of the last decode done by this decoder.
	 * 
	 * @return true, if decode was successful.
	 */
	public boolean decodeResult() { 
		return mDecodeResult;
	}
}
