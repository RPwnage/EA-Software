/**
 *  AutoTdfRegistrar.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

/**
 * The interface AutoTdfRegistrar is implemented by classes that are available to 
 * be used in variable TDF fields.  This interface is used to auto-discover and auto
 * register these classes at TdfFactory instantiation.
 */
public interface AutoTdfRegistrar {
	
	/**
	 * Registers the TDF with the TdfFactory.
	 */
	public void RegisterTdf();
}
