/**
 *  Encoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.protocol;

/**
 * The standard encoder interface.
 */
public interface Encoder {
    
    /**
	 * The enum for available encoder types.
	 */
    enum Type
    {
	    HTTP,
        PRINT,
        HEAT2,
        INVALID,
    };
    
    /**
	 * Gets the name of the encoder
	 * 
	 * @return the name of the encoder
	 */
    public abstract String getName();
    
    /**
	 * Gets the type of the encoder
	 * 
	 * @return the type of the encoder
	 */
    public abstract Type getType();
}
