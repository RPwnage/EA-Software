package com.ea.originx.automation.lib.pageobjects.login;

/**
 * Page object that represents the 'Login Options' page.
 *
 * @author palui
 */
public final class LoginOptions {

    protected Boolean rememberMe; // Tristate: null means no action
    protected Boolean invisible;  // Tristate
    protected boolean handleUnderAgeFirstLogin;
    protected boolean useEnter;
    protected boolean noWaitForDiscoverPage;
    public boolean skipVerifyingMiniProfileUsername;

    /**
     * Constructor
     */
    public LoginOptions() {
        rememberMe = null; // do nothing
        invisible = null; // do nothing
        handleUnderAgeFirstLogin = false;
        useEnter = false;
        noWaitForDiscoverPage = false;
        skipVerifyingMiniProfileUsername = false;
    }

    /**
     * Set the 'Remember Me' option for automatic login.
     *
     * @param option Option to set (null means no action)
     * @return This object (to allow chain setting)
     */
    public LoginOptions setRememberMe(Boolean option) {
        rememberMe = option;
        return this;
    }

    /**
     * Set the 'Is Invisible' option for invisible login.
     *
     * @param option Option to set (null means no action)
     * @return This object (to allow chain setting)
     */
    public LoginOptions setInvisible(Boolean option) {
        invisible = option;
        return this;
    }

    /**
     * Set the 'Handle Underage First Login' option for handling underage parent
     * email.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public LoginOptions setHandleUnderAgeFirstLogin(boolean option) {
        handleUnderAgeFirstLogin = option;
        return this;
    }

    /**
     * Set the 'Use Enter' option for submitting a login by hitting enter.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public LoginOptions setUseEnter(boolean option) {
        useEnter = option;
        return this;
    }

    /**
     * Set the 'No Wait For Discover Page' option.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public LoginOptions setNoWaitForDiscoverPage(boolean option) {
        noWaitForDiscoverPage = option;
        return this;
    }

    /**
     * Set the 'Skip Verifying Mini Profile Username' option.
     *
     * @param option Option to set
     * @return This object (to allow chain setting)
     */
    public LoginOptions setSkipVerifyingMiniProfileUsername(boolean option) {
        skipVerifyingMiniProfileUsername = option;
        return this;
    }
}