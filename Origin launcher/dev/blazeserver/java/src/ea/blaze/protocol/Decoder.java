/**
 *  Decoder.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.protocol;

/**
 * The standard decoder interface.
 */
public interface Decoder {
    
    /**
	 * The enum for available decoder types.
	 */
    enum Type
    {
        XML,
        HEAT2,
        INVALID,
    };
    
    /**
	 * Gets the name of the decoder.
	 *  
	 * @return the name of the decoder.
	 */
    public abstract String getName();
    
    /**
	 * Gets the type of the decoder.
	 * 
	 * @return the type of the decoder.
	 */
    public abstract Type getType();
}
