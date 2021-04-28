/**
 *  TdfUInt64.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf.types;

import java.math.BigInteger;

/**
 * Wrapper class for BigInteger representations for unsigned int 64 TDF values.
 */
public class TdfUInt64 implements Comparable<TdfUInt64>
{		
	
	/** The Constant TWO_TO_64. */
	private static final BigInteger TWO_TO_64 = (BigInteger.valueOf(2)).pow(64);

	/** The Constant MAX_VALUE. */
	public static final BigInteger MAX_VALUE = TWO_TO_64.subtract(BigInteger.ONE);
	
	/** The Constant MIN_VALUE. */
	public static final BigInteger MIN_VALUE = BigInteger.ZERO;
	
	/** The big integer. */
	private BigInteger mBigInteger;
	
	/** Constructs and returns a TdfUInt64 for the specified long value
	 * 
	 * @param value The value to build a UInt64 for
	 * @return The TdfUInt64
	 */
	public static TdfUInt64 valueOf(long value) {
		return new TdfUInt64(value);
	}
	
	/**
	 * Instantiates a new TdfUInt64
	 */
	public TdfUInt64() {
			mBigInteger = BigInteger.ZERO;
	}
	
	/**
	 * Instantiates a new TdfUInt64
	 * 
	 * @param intValue
	 *            the int value
	 */
	public TdfUInt64(BigInteger intValue) {
		if(intValue.compareTo(BigInteger.ZERO) == -1) {
			// negative value.  
			mBigInteger = intValue.add(TWO_TO_64);
		}
		else {
			mBigInteger = intValue;
		}
	}	
	
	/**
	 * Instantiates a new TdfUInt64.
	 * 
	 * @param longValue
	 *            the long value
	 */
	public TdfUInt64(long longValue) {
		if(longValue < 0) {
			// negative value
			mBigInteger = BigInteger.valueOf(longValue).add(TWO_TO_64);
		}
		else {
			mBigInteger = BigInteger.valueOf(longValue);
		}
	}
	
	/**
	 * Instantiates a new TdfUInt64.
	 * 
	 * @param value
	 *            the value
	 */
	public TdfUInt64(String value) {
		mBigInteger = new BigInteger(value);
		
		// Force value into range.
		mBigInteger = mBigInteger.min(MAX_VALUE);
		mBigInteger = mBigInteger.max(MIN_VALUE);
	}
	
	/**
	 * Instantiates a new TdfUInt64.
	 * 
	 * @param rhs
	 *            the rhs to copy
	 */
	public TdfUInt64(TdfUInt64 rhs) {
		mBigInteger = rhs.mBigInteger;
	}
	
	/**
	 * Gets the big integer value
	 * 
	 * @return the big integer
	 */
	public BigInteger get() {
		return mBigInteger;
	}

	
	/**
	 * Sets the big integer value
	 * 
	 * @param bigInt
	 *            the big int
	 */
	public void set(BigInteger bigInt) {
		mBigInteger = bigInt;
	}
	
	/**
	 * Sets the big integer to specified long value, casting if negative.
	 * 
	 * @param longValue
	 *            the long value
	 */
	public void set(long longValue) {
		if(longValue < 0) {
			// negative value.  In C++ we just cast the signed int64 value to the uint64 value
			// but that doesn't work here, so we add the value of the the negative bit
			mBigInteger = BigInteger.valueOf(longValue).add(TWO_TO_64);
		}
		else {
			mBigInteger = BigInteger.valueOf(longValue);
		}
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return mBigInteger.toString();
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode() {
		return mBigInteger.hashCode();
	}

	/* (non-Javadoc)
     * @see java.lang.Comparable#compareTo(java.lang.Object)
     */
    @Override
    public int compareTo(TdfUInt64 o) {
	    return mBigInteger.compareTo(o.get());
    }
}
