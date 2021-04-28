/**
 *  TdfEncoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.protocol;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import ea.blaze.protocol.Encoder;
import ea.blaze.tdf.Tdf;
import ea.blaze.tdf.Visitor;

/**
 * Abstract TdfEncoder class providing base encoder functionality.  Implementing classes
 * must implement Encoder and Visitor interfaces.
 */
public abstract class TdfEncoder implements Encoder, Visitor {
	
	/** The buffer to encode into. */
	protected ByteBuffer mByteBuffer;
	
	private static final int GROW_BUFFER_SIZE = 4096;
	
	protected boolean mOwnByteBuffer = false;
	
	/** The encode result. */
	private boolean mEncodeResult;
	
	/**
	 * Returns an OutputStream that writes directly to the underlying ByteBuffer
	 * 
	 * @return OutputStream that writes to buffer associated with encoder
	 */
	protected OutputStream getBufferAsOutputStream() {
	    return new OutputStream() {
	        public synchronized void write(int b) throws IOException {
	        	if(mByteBuffer.position() >= mByteBuffer.limit() && mOwnByteBuffer) {
	        		ByteBuffer newBuffer = ByteBuffer.allocate(mByteBuffer.capacity() + GROW_BUFFER_SIZE);
	        		mByteBuffer.flip();
	        		newBuffer.put(mByteBuffer);
	        		mByteBuffer = newBuffer;
	        	}
	        	mByteBuffer.put((byte)b);
	        }

	        public synchronized void write(byte[] bytes, int off, int len) throws IOException {
	        	if((mByteBuffer.position() + len) >= mByteBuffer.limit() && mOwnByteBuffer) {
	        		ByteBuffer newBuffer = ByteBuffer.allocate(mByteBuffer.capacity() + GROW_BUFFER_SIZE);
	        		mByteBuffer.flip();
	        		newBuffer.put(mByteBuffer);
	        		mByteBuffer = newBuffer;
	        	}
	        	mByteBuffer.put(bytes, off, len);
	        }
	    };
	}
	
	/**
	 * Encode the specified TDF into the provided buffer
	 * 
	 * @param buffer
	 *            the buffer to write TDF to
	 * @param value
	 *            the TDF to encode
	 */
	public void encode(ByteBuffer buffer, Tdf value) {
		reset();
		this.mByteBuffer = buffer;
		mEncodeResult = visit(value, value);
	}
	
	/**
	 * Allocates internal buffer that is growable via OutputStream
	 * and encodes into it, returning the buffer.
	 * 
	 * @param value The TDF to encode
	 * @return Buffer containing encoded TDF.
	 */
	public ByteBuffer encode(Tdf value) {
		reset();
		mByteBuffer = ByteBuffer.allocate(GROW_BUFFER_SIZE);
		mOwnByteBuffer = true;
		mEncodeResult = visit(value, value);
		return mByteBuffer;
	}
	
	/**
	 * Encode result.
	 * 
	 * @return true, if successful
	 */
	public boolean encodeResult() { 
		return mEncodeResult;
	}
	
	/**
	 * Sets the buffer and resets the encoder.
	 * 
	 * @param outputStream
	 *            the new buffer
	 */
	public void setBuffer(ByteBuffer buffer) {
		reset();
		this.mByteBuffer = buffer;
	}
	
	/**
	 * Reset the encoder state, does not change the state of the OutputStream buffer.
	 */
	protected abstract void reset();
	
}
