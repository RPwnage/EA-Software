package com.ea.originx.automation.lib.helpers;

import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.AccountService;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.net.URISyntaxException;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * A collection of helpers that retrieve accounts from the account manager and also creates new throwaway accounts
 * with any given parameters.
 */
public class AccountManagerHelper {

    protected static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     */
    private AccountManagerHelper() {
    }

    /**
     * Get a user account (from the account manager) entitled to one or more
     * entitlements within the specified country
     *
     * @param country The country the user is in
     * @param entitlements {@link EntitlementInfo} The entitlements to find entitled on a user account
     * @return Entitled user account if found, null otherwise
     * @throws IOException If an I/O exception occurs.
     * @throws URISyntaxException If URI to get entitlements for the account is invalid.
     */
    public static UserAccount getEntitledUserAccountWithCountry(String country, EntitlementInfo... entitlements) throws IOException, URISyntaxException {
        final Criteria criteria = new Criteria.CriteriaBuilder().country(country).build();
        for (EntitlementInfo entitlement : entitlements) {
            criteria.addEntitlement(entitlement.getName());
        }
        return AccountManager.getInstance().requestWithCriteria(criteria);
    }

    /**
     * Get a tagged account (from the account manager) for the given the tag
     *
     * @param tag AccountTags enum
     * @return A user account with the tag enum if found, null otherwise
     */
    public static UserAccount getTaggedUserAccount(AccountTags tag) {
        return AccountManager.getTaggedUserAccount(tag.toString());
    }

    /**
     * Get a tagged account (from the account manager) for the given the tag and country
     *
     * @param tag AccountTags enum
     * @param newUserCountry Country of the user
     * @return A user account with the tag enum if found, null otherwise
     */
    public static UserAccount getTaggedUserAccountWithCountry(AccountTags tag, String newUserCountry) {
        Criteria criteria = new Criteria.CriteriaBuilder().tag(tag.toString()).country(newUserCountry).build();
        return AccountManager.getInstance().requestWithCriteria(criteria, 360);
    }

    /**
     * Get a tagged account (from the account manager) for the given country
     *
     * @param newUserCountry Country of the user
     * @return The first user account with the tag enum if found, null otherwise
     */
    public static UserAccount getUserAccountWithCountry(String newUserCountry) {
        Criteria criteria = new Criteria.CriteriaBuilder().country(newUserCountry).build();
        return AccountManager.getInstance().requestWithCriteria(criteria, 360);
    }

    /**
     * Get a tagged account (from the account manager) for the given country
     * with no entitlements
     *
     * @param newUserCountry Country of the user
     * @return A user account with the tag enum if found, null otherwise
     */
    public static UserAccount getUnentitledUserAccountWithCountry(String newUserCountry) {
        return getTaggedUserAccountWithCountry(AccountTags.NO_ENTITLEMENTS, newUserCountry);
    }

    /**
     * Create a new throwaway account with the given country and user age
     *
     * @param newUserCountry Country of the user
     * @param userAge Age of the user
     * @return The newly created user account with the given country and user age
     */
    public static UserAccount createNewThrowAwayAccount(String newUserCountry, int userAge) {
        Waits.sleep(1); // Guarantee we get a unique timestamp
        return new UserAccount.AccountBuilder().age(userAge).country(newUserCountry).exists(false).build();
    }
    
    /**
     * Create a new throwaway account with the given country and user first name
     *
     * @param newUserCountry Country of the user
     * @param firstname First name of the user
     * @return The newly created user account with the given country and user first name
     */
    public static UserAccount createNewThrowAwayAccountFirstName(String newUserCountry, String firstname) {
        return new UserAccount.AccountBuilder().firstName(firstname).country(newUserCountry).exists(false).build();
    }
    
    /** 
     * Create a new throwaway account with the given country
     *
     * @param newUserCountry Country of the user
     * @return The newly created user account with the given country
     */
    public static UserAccount createNewThrowAwayAccount(String newUserCountry) {
        Waits.sleep(1); // Guarantee we get a unique timestamp
        return new UserAccount.AccountBuilder().country(newUserCountry).exists(false).build();
    }

    /**
     * Create a new throwaway account with the given user age
     *
     * @param userAge Age of the user
     * @return The newly created user account with the given user age
     */
    public static UserAccount createNewThrowAwayAccount(int userAge) {
        Waits.sleep(1); // Guarantee we get a unique timestamp
        return new UserAccount.AccountBuilder().age(userAge).exists(false).build();
    }

    /**
     * Register a new (adult) user account through REST. Default country is Canada.
     *
     * @return The new user account registered through REST
     *
     * @throws IOException If an I/O exception occurs.
     */
    public static UserAccount registerNewThrowAwayAccountThroughREST() throws IOException {
        UserAccount account = createNewThrowAwayAccount("Canada");
        AccountService.registerNewUserThroughREST(account, null);
        return account;
    }

    /**
     * Register a new (adult) user account through REST
     *
     * @param newUserCountry Country of the user
     * @return The new user account registered through REST
     *
     * @throws IOException If an I/O exception occurs.
     */
    public static UserAccount registerNewThrowAwayAccountThroughREST(String newUserCountry) throws IOException {
        UserAccount account = createNewThrowAwayAccount(newUserCountry);
        AccountService.registerNewUserThroughREST(account, null);
        return account; 
    }

    /**
     * Register a new (adult) user account through REST, if registering fails retry
     *
     * @param newUserCountry Country of the user
     * @return The new user account registered through REST
     *
     */
    public static UserAccount registerNewThrowAwayAccountRetryThroughREST(String newUserCountry, int retryAmount) {

        boolean registered = false;
        UserAccount account = null;

        while (retryAmount > 0 && !registered) {
            account = createNewThrowAwayAccount(newUserCountry);

            try {
                AccountService.registerNewUserThroughREST(account, null);
                registered = true;
            } catch (Exception e) {
                _log.warn(e.getMessage());
            }
            retryAmount--;
        }

        if (retryAmount == 0 && !registered) {
            throw new RuntimeException(">>> FAILED : Cannot register a new user");
        }

        return account;
    }

    /**
     * Register a new user account through REST, given its age and whether it is
     * an underage user
     *
     * @param age New user's age
     * @param parentalEmail Parent email address
     *
     * @return The new user account registered through REST
     *
     * @throws IOException If an I/O exception occurs.
     */
    public static UserAccount registerNewUnderAgeThrowAwayAccountThroughREST(int age, String parentalEmail) throws IOException {
        if (parentalEmail == null) {
            throw new RuntimeException("parentalEmail cannot be null");
        }
        UserAccount account = createNewThrowAwayAccount(age);
        AccountService.registerNewUserThroughREST(account, parentalEmail);
        return account;
    }
    
    /**
     * Register a new user account through REST, given its age and country and
     * whether it is an underage user
     *
     * @param age New user's age
     * @param country New user's country
     * @param parentalEmail Parent email address
     *
     * @return The new user account registered through REST
     *
     * @throws IOException If an I/O exception occurs.
     */
    public static UserAccount registerNewUnderAgeWithCountryThrowAwayAccountThroughREST(int age, String country, String parentalEmail) throws IOException {
        if (parentalEmail == null) {
            throw new RuntimeException("parentalEmail cannot be null");
        }
        UserAccount account = createNewThrowAwayAccount(country, age);
        AccountService.registerNewUserThroughREST(account, parentalEmail);
        return account;
    }

}
