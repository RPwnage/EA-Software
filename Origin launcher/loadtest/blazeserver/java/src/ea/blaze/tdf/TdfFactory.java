/**
 *  TdfFactory.java
 *
 *  (c) Electronic Arts. All Rights Reserved.
 *
 */
package ea.blaze.tdf;

import java.util.HashMap;
import java.util.ServiceLoader;

/**
 * A factory for creating Tdf objects from a TDF ID to class name.
 * In particular this factory can be used to construct TDFs from variable TDF information.
 */
public class TdfFactory {

	/**
	 * Anonymous instances of this interface are provided by auto-generated TDFs to register themselves with the factory.
	 */
	public static interface TdfCreator {
		
		/**
		 * Creates a TDF and returns it.
		 * 
		 * @return the created TDF
		 */
		public Tdf createTdf();
	}
	
	/**
	 * The TdfRegistration class is used to store information about each TDF that is registered with the factory.
	 */
	private static class TdfRegistration
	{
		
		/** The TDF creator method to construct TDFs of this type. */
		private TdfCreator mTdfCreator;
		
		/** The TDF class name. */
		private String mTdfClassName;
		
		/** The TDF id. */
		private Long mTdfId;
		
		/**
		 * Instantiates a new TDF registration.
		 * 
		 * @param tdfClassName
		 *            the TDF class name
		 * @param tdfId
		 *            the TDF id
		 * @param tdfCreator
		 *            the TDF creator
		 */
		public TdfRegistration(String tdfClassName, Long tdfId, TdfCreator tdfCreator) {
			mTdfClassName = tdfClassName;
			mTdfCreator = tdfCreator;
			mTdfId = tdfId;
		}
		
		/**
		 * Gets the TDF class name.
		 * 
		 * @return the TDF class name
		 */
		public String getTdfClassName() {
			return mTdfClassName;
		}
		
		/**
		 * Gets the TDF id.
		 * 
		 * @return the TDF id
		 */
		public Long getTdfId() {
			return mTdfId;
		}
		
		/**
		 * Gets the TDF creator.
		 * 
		 * @return the TDF creator
		 */
		public TdfCreator getTdfCreator() {
			return mTdfCreator;
		}
	}
	
	/** The singleton instance of the TDF factory */
	private static TdfFactory instance;
	
	/**
	 * This static block initializes the factory and then uses Java's ServiceLoader to find
	 * all auto-generated TDFs that implement the AutoTdfRegsitrar interface and call their
	 * method to register themselves with the factory.  This allows us to work around the
	 * fact that static initialization doesn't happen for Java classes until they are loaded.
	 */
	static {
		instance = new TdfFactory();
		ServiceLoader<AutoTdfRegistrar> services = ServiceLoader.load(AutoTdfRegistrar.class);
		for(AutoTdfRegistrar registrar : services) {
			registrar.RegisterTdf();
		}
	}
	
	/** The TDF registration map. */
	private static HashMap<Long, TdfRegistration> mTdfRegistrationMap;
	
	/** The TDF name to TDF id map. */
	private HashMap<String, Long> mTdfNameToId;
	
	/**
	 * Gets the singleton TDF factory.
	 * 
	 * @return the TDF factory
	 */
	public static TdfFactory get() {
		return instance;
	}
	
	/**
	 * Instantiates a new TDF factory.
	 */
	private TdfFactory() {
		mTdfRegistrationMap = new HashMap<Long, TdfRegistration>();
		mTdfNameToId = new HashMap<String, Long>();
	}
	
	/**
	 * Register TDF with the factory.
	 * 
	 * @param tdfClassName
	 *            the TDF class name
	 * @param tdfId
	 *            the TDF id
	 * @param tdfCreator
	 *            the TDF creator method
	 * @return true, if registration was successful
	 */
	public boolean registerTdf(String tdfClassName, Long tdfId, TdfCreator tdfCreator) {
		
		if(tdfId == Tdf.INVALID_TDF_ID) {
			return false;
		}
		else if(mTdfRegistrationMap.containsKey(tdfId)) {
			return false;
		}
		else {
			TdfRegistration registration = new TdfRegistration(tdfClassName, tdfId, tdfCreator);
			mTdfRegistrationMap.put(registration.getTdfId(), registration);
			mTdfNameToId.put(registration.getTdfClassName(), registration.getTdfId());
			return true;
		}
	}
	
	/**
	 * Deregister the TDF from the factory.  Removes this TDF from possible TDFs this factory can create.
	 * 
	 * @param tdfId
	 *            the tdf id to remove
	 * @return true, if TDF was found and removed.  False if TDF had not been registered.
	 */
	public boolean deregisterTdf(Long tdfId) {
		if(mTdfRegistrationMap.containsKey(tdfId)) {
			TdfRegistration reg = mTdfRegistrationMap.remove(tdfId);
			mTdfNameToId.remove(reg.getTdfClassName());
			return true;
		}
		else {
			return false;
		}
	}
	
	/**
	 * Creates a new Tdf object for the specified TDF ID, if TDF is registered with factory.
	 * 
	 * @param tdfId
	 *            the tdf id of TDF to create.
	 * @return the created TDF if TDF ID is valid and registered, otherwise null.
	 */
	public Tdf createTdf(Long tdfId) {
		if(mTdfRegistrationMap.containsKey(tdfId)) {
			return mTdfRegistrationMap.get(tdfId).getTdfCreator().createTdf();
		}
		return null;
	}
	
	/**
	 * Creates a new Tdf object for the specified TDF class name, if TDF is registered with factory.
	 * 
	 * @param className
	 *            the class name of TDF to create.
	 * @return the created TDF if class name is a valid TDF and registered, otherwise null.
	 */
	public Tdf createTdf(String className) {
		if(mTdfNameToId.containsKey(className)) {
			return createTdf(mTdfNameToId.get(className));
		}
		else {
			return null;
		}
	}
}
