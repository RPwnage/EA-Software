package com.ea.originx.automation.lib.helpers;

import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.net.helpers.RestfulHelper;
import com.ea.vx.originclient.utils.Waits;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Properties;
import javax.annotation.Nullable;
import javax.mail.Folder;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Session;
import javax.mail.Store;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

/**
 * Fetcher for emails
 * Currently only support retrieval from the Gmail multi-user account(eatestoriginautomation@gmail.com).
 * Username is used to locate emails for a specific user (email address: eatestoriginautomation+'username'@gmail.com).
 *
 * @author mkalaivanan
 * @author palui
 */
public class EmailFetcher {

    public static final String GMAIL_EMAIL_ADDRESS = "eatestoriginautomation@gmail.com";
    public static final String GMAIL_PASSWORD = "Qat3st3r";

    // Messages from "EA <ea@e.ea.com>"
    public static final String EMAIL_SUBJECT_WELCOME = "Welcome to Your EA Account";
    public static final String EMAIL_SUBJECT_VERIFY = "Verify Your Account Email";
    public static final String EMAIL_SUBJECT_RESET_PASSWORD = "Reset Your Password";
    public static final String EMAIL_SUBJECT_TWO_FACTOR_VERFICATION_ON = "Your EA Login Verification is ON";
    public static final String EMAIL_SUBJECT_TWO_FACTOR_VERFICATION_OFF = "Your EA Login Verification is OFF";
    public static final String EMAIL_SUBJECT_SECURITY_CODE = "Your EA Security Code is:";
    public static final String EMAIL_SUBJECT_PASSWORD_UPDATED = "Your Password Has Been Updated";
    public static final String EMAIL_SUBJECT_PARENTAL_NOTIFICATION = "Parental Notification of Limited Origin Account Creation";

    // Messages from "Origin <noreply@e.ea.com>"
    public static final String EMAIL_SUBJECT_ORDER_CONFIRMATION = "Order Confirmation";
    public static final String EMAIL_SUBJECT_WELCOME_TO_ORIGIN_ACCESS = "Welcome to Origin Access - endless gaming awaits";
    public static final String EMAIL_SUBJECT_ORIGIN_ACCESS_CANCELLED = "Your Origin Access membership has been cancelled";
    public static final String EMAIL_SUBJECT_PREORDER_NOW_AVAILABLE = "Preorder Now Available";

    // Messages for gifting
    public static final String EMAIL_SUBJECT_GIFT_SENT = "Receipt for your gift purchase on Origin";
    public static final String EMAIL_SUBJECT_GIFT_RECEIVED = "You have a gift waiting for you on Origin";

    // Parent email
    public static final String[] VERIFY_PARENT_EMAIL_LINK_KEYWORDS = {"verify", "parent", "email address"};

    //Messages for OA membership email
    public static final String EMAIL_SUBJECT_MEMBERSHIP_STARTS_NOW = "Your Origin Access membership starts now";


    // Gift recipient content
    private final String username;
    private final String emailAddress;
    private final String password;
    private Folder inbox;
    private Store store;

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor - use default emailAddress and password.
     *
     * @param username Username for locating a user's emails in the multi-user Gmail account
     */
    public EmailFetcher(String username) {
        this.username = username;
        this.emailAddress = GMAIL_EMAIL_ADDRESS;
        this.password = GMAIL_PASSWORD;
        this.inbox = null;
        this.store = null;
    }

    /**
     * Gets the inbox.
     *
     * @return The inbox object
     */
    public Folder getInbox() {
        return inbox;
    }

    /**
     * Sets the inbox for Gmail.
     *
     * @param inBox Folder to set the inbox to
     */
    public void setInbox(Folder inBox) {
        this.inbox = inBox;
    }

    /**
     * Get the store object.
     *
     * @return {@link Store} The store object
     */
    public Store getStore() {
        return store;
    }

    /**
     * Set the store object.
     *
     * @param store The store object
     */
    public void setStore(Store store) {
        this.store = store;
    }

    /**
     * Set the Imap and SSL properties required to connect to the Gmail server.
     *
     * @return Properties required to establish connection with the Gmail server
     * for retrieving emails
     */
    private static Properties getGmailImapSslProperties() {
        final String portNumber = "993";
        Properties properties = new Properties();
        properties.put("mail.imap.host", "imap.gmail.com");
        properties.put("mail.imap.port", portNumber);
        // SSL setting
        properties.setProperty("mail.imap.socketFactory.class",
                "javax.net.ssl.SSLSocketFactory");
        properties.setProperty("mail.imap.socketFactory.fallback", "false");
        properties.setProperty("mail.imap.socketFactory.port",
                String.valueOf(portNumber));
        return properties;
    }

    /**
     * Close the connection to the Gmail server.
     *
     * @throws MessagingException if error occurs while closing connection.
     */
    public void closeConnection() throws MessagingException {
        if (inbox != null) {
            inbox.close(true);
        }
        if (store != null) {
            store.close();
        }
    }

    /**
     * Get the 20 latest emails (all users) from the multi-user Gmail account.
     *
     * @return An array of 20 messages received, null if none found
     * @throws MessagingException If an error occurs while closing connection.
     */
    public Message[] getLatestEmails() throws MessagingException {

        // Use only the first 20 emails as the account has thousands of emails and
        // we are only interested in the most recent emails
        int numberOfEmailsToRetrieve = 20;

        Session session = Session.getDefaultInstance(getGmailImapSslProperties());
        setStore(session.getStore("imap"));
        store.connect(emailAddress, password);
        setInbox(store.getFolder("inbox"));
        inbox.open(Folder.READ_ONLY);
        int count = inbox.getMessageCount();
        // Latest messages are inserted at the end
        Message[] firstSetOfEmails = inbox.getMessages((count - numberOfEmailsToRetrieve), count);
        return firstSetOfEmails;
    }

    /**
     * Get the latest emails for this user with matching subject.
     *
     * @param expectedSubject subject to match
     * @return Email messages for the user with matching subject, or empty list if not found
     * @throws MessagingException If error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public ArrayList<Message> getEmailsWithMatchingSubject(String expectedSubject) throws MessagingException, IOException {
        Message[] messageList = getLatestEmails();
        ArrayList<Message> matches = new ArrayList<>();
        // Traverse backwards to retrieve the latest emails first
        for (int i = messageList.length - 1; i >= 0; i--) {
            Message message = messageList[i];
            if (StringHelper.containsIgnoreCase(message.getAllRecipients()[0].toString(), username)) {
                if (StringHelper.containsIgnoreCase(message.getSubject(), expectedSubject)) {
                    matches.add(message);
                }
            }
        }
        return matches;
    }

    /**
     * Get the latest email for this user with matching subject.
     *
     * @param expectedSubjectText Subject to match
     * @return Latest email message for the user with matching subject, or null if not found
     * @throws MessagingException If error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Message getLatestEmailWithMatchingSubject(String expectedSubjectText) throws MessagingException, IOException {
        Message[] messageList = getLatestEmails();
        // Traverse backwards to retrieve the latest emails first
        for (int i = messageList.length - 1; i >= 0; i--) {
            Message message = messageList[i];
            if (StringHelper.containsIgnoreCase(message.getAllRecipients()[0].toString(), username)) {
                if (StringHelper.containsIgnoreCase(message.getSubject(), expectedSubjectText)) {
                    return message;
                }
            }
        }
        return null;
    }

    /**
     * Get the latest 'Reset Your Password' email for this user.
     *
     * @return {@link Message} Latest 'Reset Your Password' message for this
     * user, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Message getLatestResetPasswordEmail() throws MessagingException, IOException {
        return getLatestEmailWithMatchingSubject(EMAIL_SUBJECT_RESET_PASSWORD);
    }

    /**
     * Get the reset password link from the 'Reset Your Password' email for this user.
     *
     * @return Reset password link matching the LOGIN_PAGE_URL_REGEX, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public String getResetPasswordLink() throws MessagingException, IOException {
        String errorMessage = "Cannot find password reset email";
        try {
            boolean gotResetPasswordEmail
                    = Waits.pollingWaitEx(() -> getLatestResetPasswordEmail() != null, 300000, 10000, 0);
            if (gotResetPasswordEmail) {
                Message message = getLatestResetPasswordEmail();
                String contentType = message.getContentType();
                if (contentType.contains("TEXT/HTML")) {
                    Object content = message.getContent();
                    if (content != null) {
                        // Since content is html content present in string parse it using JSoup
                        Document doc = Jsoup.parse(content.toString());
                        return doc.getElementsMatchingText(OriginClientData.LOGIN_PAGE_URL_REGEX).get(0).text();
                    }
                }
            } else {
                _log.error(errorMessage);
                throw new RuntimeException(errorMessage);
            }
        } catch (IOException | MessagingException e) {
            _log.error(errorMessage);
            throw new RuntimeException(e);
        }
        return null;
    }

    /**
     * Get the latest 'Parental Notification of Limited Origin Account Creation'
     * email from parent's email address for this (underage) user. Assumption is
     * the parent's email will contain the underage username as part of the recipient list.
     *
     * @return The latest 'Parental Notification of Limited Origin Account Creation'
     * message for this (underage) user, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Message getLatestUnderageParentalNotificationEmail() throws MessagingException, IOException {
        return getLatestEmailWithMatchingSubject(EMAIL_SUBJECT_PARENTAL_NOTIFICATION);
    }

    /**
     * Get the first "a[href]" link with text matching expected link text keywords.
     *
     * @param message The email message containing links
     * @param expectedLinkTextKeywords The expected link text keywords
     * @return Link with expected text keywords, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public String getLinkWithMatchingKeywords(Message message, String[] expectedLinkTextKeywords) throws MessagingException, IOException {
        String contentType = message.getContentType();
        if (contentType.contains("TEXT/HTML")) {
            Object content = message.getContent();
            if (content != null) {
                Document doc = Jsoup.parse(content.toString());
                Elements links = doc.select("a[href]");
                for (Element link : links) {
                    if (StringHelper.containsIgnoreCase(link.text(), expectedLinkTextKeywords)) {
                        return link.attr("href");
                    }
                }
            }
        }
        return null;
    }

    /**
     * Get the 'Verify Parent Email' link from the parent's 'Parental
     * Notification of Limited Origin Account Creation' email for this
     * (underage) user.
     *
     * @return 'Verify Parent Email' link, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public @Nullable
    String getVerifyParentEmailLink() throws IOException, MessagingException {
        String errorMessage = "Cannot find 'Verify Parent Email' link email";
        try {
            boolean gotParentNotificationEmail = Waits.pollingWaitEx(()
                    -> getLatestUnderageParentalNotificationEmail() != null, 300000, 10000, 0);
            if (gotParentNotificationEmail) {
                return getLinkWithMatchingKeywords(getLatestUnderageParentalNotificationEmail(), VERIFY_PARENT_EMAIL_LINK_KEYWORDS);
            } else {
                _log.error(errorMessage);
                throw new RuntimeException(errorMessage);
            }
        } catch (IOException | MessagingException e) {
            _log.error(errorMessage);
            throw new RuntimeException(e);
        }
    }

    /**
     * Get the latest 'Your EA Security Code is:' email for this user.
     *
     * @return latest 'Your EA Security Code is:' message for this user, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Message getLatestSecurityCodeEmail() throws MessagingException, IOException {
        return getLatestEmailWithMatchingSubject(EMAIL_SUBJECT_SECURITY_CODE);
    }

    /**
     * Get the security code from the latest 'Your EA Security Code is:' email
     * for this user . The code appears in the subject of the email.
     *
     * @return The security code
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public String getLatestSecurityCode() {
        String errorMessage = "Cannot find security code email";
        try {
            boolean gotSecurityCodeEmail = Waits.pollingWaitEx(() -> getLatestSecurityCodeEmail() != null, 300000, 10000, 0);
            if (gotSecurityCodeEmail) {
                return StringHelper.removeNonDigits(getLatestSecurityCodeEmail().getSubject());
            } else {
                _log.error(errorMessage);
                throw new RuntimeException(errorMessage);
            }
        } catch (IOException | MessagingException e) {
            _log.error(errorMessage);
            throw new RuntimeException(e);
        }
    }

    /**
     * Get the latest 'Gift Sent' or 'Gift Received' email for this user.
     *
     * @param giftSent True if email is for sending a gift, false if the email is for receiving a gift
     * @return Latest 'Gift' message for this user, or throw RuntimeException if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Message getLatestGiftEmail(boolean giftSent) {
        String errorMessage = "Cannot find gift " + (giftSent ? "sent" : "received") + " email";
        String emailSubject = giftSent ? EMAIL_SUBJECT_GIFT_SENT : EMAIL_SUBJECT_GIFT_RECEIVED;
        try {
            boolean gotGiftEmail = Waits.pollingWaitEx(
                    () -> getLatestEmailWithMatchingSubject(emailSubject) != null, 300000, 10000, 0);
            if (gotGiftEmail) {
                return getLatestEmailWithMatchingSubject(emailSubject);
            } else {
                _log.error(errorMessage);
                throw new RuntimeException(errorMessage);
            }
        } catch (IOException | MessagingException e) {
            _log.error(errorMessage);
            throw new RuntimeException(e);
        }
    }

    /**
     * Get the latest 'Origin Access new subscription starts now' email for this user.
     *
     * @return Latest 'OA New Subscription' message for this user, or throw RuntimeException if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Message getLatestNewOAMembershipEmail() {
        String errorMessage = "Cannot find Origin Access new subscription starts now email";
        String emailSubject = EMAIL_SUBJECT_MEMBERSHIP_STARTS_NOW;
        try {
            boolean gotNewSubscriptionEmail = Waits.pollingWaitEx(
                    () -> getLatestEmailWithMatchingSubject(emailSubject) != null, 300000, 10000, 0);
            if (gotNewSubscriptionEmail) {
                return getLatestEmailWithMatchingSubject(emailSubject);
            } else {
                _log.error(errorMessage);
                throw new NullPointerException(errorMessage);
            }
        } catch (IOException | MessagingException e) {
            _log.error(errorMessage);
            throw new RuntimeException(e);
        }
    }

    /**
     * Get the latest 'Gift Sent' email message for this user.
     *
     * @return The latest 'Gift Sent' email message for this user, or throw
     * RuntimeException if not found
     */
    public Message getLatestGiftSentEmail() {
        return getLatestGiftEmail(true);
    }

    /**
     * Get the latest 'Gift Received' email message for this user.
     *
     * @return latest 'Gift Received' email message for this user, or throw
     * RuntimeException if not found
     */
    public Message getLatestGiftReceivedEmail() {
        return getLatestGiftEmail(false);
    }

    /**
     * From a email message, retrieve element with specified selector which text
     * matches the keywords.
     *
     * @param message Email message
     * @param selector Selector (CSS style) to locate the element
     * @param keywords Expected keywords of the element's text
     * @return element with specified selector which text has the matching
     * keywords, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public static @Nullable
    Element getElementMatchingKeywords(Message message, String selector, String[] keywords) throws MessagingException, IOException {
        String contentType = message.getContentType();
        if (contentType.contains("TEXT/HTML")) {
            Object content = message.getContent();
            if (content != null) {
                Document doc = Jsoup.parse(content.toString());
                Elements elements = doc.select(selector);
                for (Element element : elements) {
                    if (StringHelper.containsIgnoreCase(element.text(), keywords)) {
                        return element;
                    }
                }
            }
        }
        return null;
    }

    /**
     * Get field values from the latest 'Gift Received' email.
     *
     * @return {@link Map} of the retrieved fields ("Order Number", "Sender", "Gift
     * Message", "Open Your Gift Link").
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Map<String, String> getLatestGiftReceivedEmailFields() throws MessagingException, IOException {
        String[] EXPECTED_ORDER_NUMBER_KEYWORDS = {"Order", "Number"};
        String[] EXPECTED_GIFT_MESSAGE_KEYWORDS = {"Just", "Sent", "You", "Gift"};
        String[] EXPECTED_OPEN_YOUR_GIFT_LINK_KEYWORDS = {"Open Your Gift"};
        Map<String, String> result = new LinkedHashMap<>();
        Message message = getLatestGiftReceivedEmail();

        // Get the "Order Number: <number>" table row detail from the email
        Element element = getElementMatchingKeywords(message, "td[align=right]", EXPECTED_ORDER_NUMBER_KEYWORDS);
        if (element != null) {
            String orderNumberRow = element.text();
            String[] orderNumberTokens = orderNumberRow.split(" ");
            String orderNumber = orderNumberTokens[orderNumberTokens.length - 1];
            result.put("Order Number", orderNumber);
        }

        // Get the element with "<sender> Just Sent You a Gift" from the email
        element = getElementMatchingKeywords(message, "span[class*=TITLE]", EXPECTED_GIFT_MESSAGE_KEYWORDS);
        if (element != null) {
            String giftMessage = element.text();
            String sender = giftMessage.split(" ", 2)[0];
            result.put("Sender", sender);
            result.put("Gift Message", giftMessage);
        }

        // Get the element with "Open Your Gift" from the email
        element = getElementMatchingKeywords(message, "a[href]", EXPECTED_OPEN_YOUR_GIFT_LINK_KEYWORDS);
        if (element != null) {
            String openYourGiftLink = element.attr("href");
            result.put("Open Your Gift Link", openYourGiftLink);
        }

        _log.info("Gift received email fields: " + result.toString());
        return result;
    }

    /**
     * Get the 'Order Number' from the email.
     *
     * @return {@link Map} of the order number
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getOrderNumberField() throws MessagingException, IOException {
        String[] EXPECTED_ORDER_NUMBER_KEYWORDS = {"Order", "Number"};
        String orderNumber;
        Map<String, String> result = new LinkedHashMap<>();
        Element element = getElementMatchingKeywords(getLatestGiftSentEmail(), "td[align=right]", EXPECTED_ORDER_NUMBER_KEYWORDS);
        if (element != null) {
            String orderNumberRow = element.text(); // suppose to get string like this 'Order Number: 1010341341828'
            String[] orderNumberTokens = orderNumberRow.split(" "); //split that strings like this, 'Order','Number:','1010341341828'
            orderNumber = orderNumberTokens[orderNumberTokens.length - 1]; //get last String which is order number like '1010341341828'
            result.put("Order Number", orderNumber);
        }
        return result;
    }

    /**
     * Get the 'Order Date' from the email.
     *
     * @return {@link Map} of the order date
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getOrderDateField() throws MessagingException, IOException {
        String[] EXPECTED_ORDER_DATE_KEYWORDS = {"Order", "Date"};
        Element element = getElementMatchingKeywords(getLatestGiftSentEmail(), "td[align=right]", EXPECTED_ORDER_DATE_KEYWORDS);
        Map<String, String> result = new LinkedHashMap<>();
        if (element != null) {
            String orderDateRow = element.text(); //suppose to get string like this 'Order Date: 10/11/16'
            String[] orderDateTokens = orderDateRow.split(" "); //split that string like this, 'Order','Date:','10/11/16'
            String orderDate = orderDateTokens[orderDateTokens.length - 1]; //get last String which is order date like '10/11/16'
            result.put("Order Date", orderDate);
        }
        return result;
    }

    /**
     * Get the gift sender's email address.
     *
     * @return {@link Map} of the gift sender's email addresses
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getGiftSenderEmailField() throws MessagingException, IOException {
        String[] EXPECTED_SENDER_EMAIL_KEYWORD = {"@"};
        Elements elements = getElementsMatchingSelector(getLatestGiftSentEmail(), "td[align=center]", EXPECTED_SENDER_EMAIL_KEYWORD);
        Map<String, String> result = new LinkedHashMap<>();
        if (elements != null && elements.size() > 0) {
            String senderEmailAddr = elements.get(elements.size() - 1).text(); // last element has only email info, others contains texts with email info
            result.put("Sender Email", senderEmailAddr);
        }
        return result;
    }

    /**
     * Get the name of the gift receiver table row from the email.
     *
     * @return {@link Map} of the gift receiver's table row
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getGiftSentToMessageField() throws MessagingException, IOException {
        String[] EXPECTED_GIFT_SENT_TO_MESSAGE_KEYWORDS = {"gift", "sent", "to"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestGiftSentEmail(), "td[align=center]", EXPECTED_GIFT_SENT_TO_MESSAGE_KEYWORDS);
        if (elements != null && elements.size() >= 3) {
            String giftSentToMessage = elements.get(2).text(); // 3rd element contains gift sent to (gift receiver name) message
            String giftReceiver = elements.get(elements.size() - 1).text(); // last element contains receiver name
            String[] giftReceiverTokens = giftReceiver.split(" ");
            String giftReceiverName = giftReceiverTokens[giftReceiverTokens.length - 1];
            if (StringHelper.containsAnyIgnoreCase(giftSentToMessage, "sent to")) {
                result.put("GiftSentTo Message", giftSentToMessage);
            }
            result.put("Gift Receiver", giftReceiverName);
        }

        return result;
    }

    /**
     * Get order summary element.
     *
     * @return {@link Map} of the order summary element
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getOrderSummaryTextField() throws MessagingException, IOException {
        String[] EXPECTED_ORDER_SUMMARY_KEYWORDS = {"Order", "Summary"};
        Map<String, String> result = new LinkedHashMap<>();
        Element element = getElementMatchingKeywords(getLatestGiftSentEmail(), "span[class=mobileTITLE1]", EXPECTED_ORDER_SUMMARY_KEYWORDS);
        if (element != null) {
            String orderSummaryText = element.text();
            if (StringHelper.containsIgnoreCase(orderSummaryText, "Order Summary")) {
                result.put("Order Summary", orderSummaryText);
            }
        }
        return result;
    }

    /**
     * Get the name of the entitlement from the 'Gift Sent' email.
     *
     * @return {@link Map} of the name of the entitlement from the 'Gift Sent' email
     * @throws MessagingException If error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getEntitlementSentField() throws MessagingException, IOException {
        String[] EXPECTED_TO_DOWNLOAD_YOUR_GAME_KEYWORDS = {"To", "download", "your", "game"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestGiftSentEmail(), "td[align=left][class=body-text]");
        if (elements != null && elements.size() > 0) {
            result.put("Entitlement Name", elements.get(elements.size() - 1).text().trim());
        }
        return result;
    }

    /**
     * Get the email footer text.
     *
     * @return {@link Map} of the footer text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getFooterField() throws MessagingException, IOException {
        String[] EXPECTED_FOOTER_KEYWORDS = {"EA", "is", "subject"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestGiftSentEmail(), "td[align=center]", EXPECTED_FOOTER_KEYWORDS);
        if (elements != null && elements.size() > 0) {
            String footerText = elements.get(elements.size() - 1).text(); // get all parent and child elements
            if (StringHelper.containsIgnoreCase(footerText, "Electronic Arts Inc.")) { // get only child item which contains 'Electronic Arts Inc.
                result.put("Footer", footerText);
            }
        }
        return result;
    }

    /**
     * Get the email subject line text.
     *
     * @return {@link Map} of the subject line text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getSubjectLineField() throws MessagingException, IOException {
        String[] expectedSubjectLineKeywords = {"Welcome", "to", "Origin", "Access!"};
        Map<String, String> result = new LinkedHashMap<>();
        Element element = getElementMatchingKeywords(getLatestNewOAMembershipEmail(), "span[class=mobileTITLE1]", expectedSubjectLineKeywords);
        if (element != null) {
            result.put("Subject Line", element.text());
        }
        return result;
    }

    /**
     * Get the email seller address text.
     *
     * @return {@link Map} of the seller address text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getSellerAddressField() throws MessagingException, IOException {
        String[] expectedSellerAndAddressKeywords = {"Seller", "Electronic", "Arts", "Redwood", "Shores", "Redwood", "City", "94065"};
        Map<String, String> result = new LinkedHashMap<>();
        Element element = getElementMatchingKeywords(getLatestNewOAMembershipEmail(), "td[align=center]", expectedSellerAndAddressKeywords);
        if (element != null) {
            result.put("Seller Address", element.text());
        }
        return result;
    }

    /**
     * Get the email EA id username text.
     *
     * @return {@link Map} of the EA id username text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getEAIdUsernameField() throws MessagingException, IOException {
        String[] expectedEaIdUsernameKeywords = {"ID:"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestNewOAMembershipEmail(), "td[class=body-text]");
        if (elements != null) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, expectedEaIdUsernameKeywords)) {
                    result.put("EA Id Username", elements.get(i + 1).text().trim()); // get '$(ID)' from next element of 'ID'
                }
            }
        }
        return result;
    }

    /**
     * Get the email payment method text.
     *
     * @return {@link Map} of the payment method text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getPaymentMethodField() throws MessagingException, IOException {
        String[] expectedPaymentMethodKeywords = {"Payment", "Method:"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestNewOAMembershipEmail(), "td[class=body-text]");
        if (elements != null && elements.size() > 0) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, expectedPaymentMethodKeywords)) {
                    result.put("Payment method", elements.get(i + 1).text().trim()); // get '$(paymentmethod)' from next element of 'Payment method'
                }
            }
        }
        return result;
    }

    /**
     * Get the email sign in to origin link text.
     *
     * @return {@link Map} of the sign in to origin text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getSignInToOriginField() throws MessagingException, IOException {
        String[] expectedSignInToOriginKeywords = {"Sign", "in", "to", "Origin"};
        Map<String, String> result = new LinkedHashMap<>();
        Element element = getElementMatchingKeywords(getLatestNewOAMembershipEmail(), "a[href]", expectedSignInToOriginKeywords);
        if (element != null) {
            String signIntoOriginLink = element.attr("href");
            result.put("Sign in URL", signIntoOriginLink);
        }
        return result;
    }

    /**
     * Get the email 'download it here' link text.
     *
     * @return {@link Map} of the download it here text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getDownloadItHereField() throws MessagingException, IOException {
        String[] expectedDownloadItHereKeywords = {"download", "it", "here"};
        Map<String, String> result = new LinkedHashMap<>();
        Element element = getElementMatchingKeywords(getLatestNewOAMembershipEmail(), "a[href]", expectedDownloadItHereKeywords);
        if (element != null) {
            String downloadItHereLink = element.attr("href");
            result.put("Download it here", downloadItHereLink);
        }
        return result;
    }

    /**
     * Get the email 'ea account' link text.
     *
     * @return {@link Map} of the EA account text
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getEAAccountField() throws MessagingException, IOException {
        String[] expectedEaAccountKeywords = {"EA", "Account"};
        Map<String, String> result = new LinkedHashMap<>();
        Element element = getElementMatchingKeywords(getLatestNewOAMembershipEmail(), "a[href]", expectedEaAccountKeywords);
        if (element != null) {
            String EAAccountLink = element.attr("href");
            result.put("EA Account", EAAccountLink);
        }
        return result;
    }

    /**
     * Get the 'Subtotal', 'Tax', and 'Total Price' values.
     *
     * @return {@link Map} of the 'Subtotal', 'Tax', and 'Total Price' values.
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getSubTotalTaxTotalPriceField() throws MessagingException, IOException {
        String[] EXPECTED_SUBTOTAL_KEYWORD = {"Subtotal"};
        String[] EXPECTED_TAX_KEYWORD = {"Tax"};
        String[] EXPECTED_TOTAL_PRICE_KEYWORDS = {"Total", "Price"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestGiftSentEmail(), "td[class=body-text]");
        if (elements != null && elements.size() > 0) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, EXPECTED_SUBTOTAL_KEYWORD)) {
                    result.put("Subtotal", elements.get(i + 1).text().trim()); // get '$(price)' from next element of 'Subtotal'
                }
                if (StringHelper.containsIgnoreCase(text, EXPECTED_TAX_KEYWORD) && !StringHelper.containsIgnoreCase(text, "taxes")) {
                    result.put("Tax", elements.get(i + 1).text().trim()); // get '$(price)' from next element of 'Tax'
                }
                if (StringHelper.containsIgnoreCase(text, EXPECTED_TOTAL_PRICE_KEYWORDS)) {
                    result.put("Total Price", elements.get(i + 1).text().trim()); // get '$(price)' from next element of 'Total Price'
                }
            }
        }
        return result;
    }

    /**
     * Get the 'Subtotal', 'Tax', and 'Total Price' values from a specific e-mail.
     *
     * @param email Email message
     * @return {@link Map} of the 'Subtotal', 'Tax', and 'Total Price' values.
     * @throws MessagingException if error occurs while closing connection.
     * @throws IOException if an I/O exception occurs.
     */
    private Map<String, String> getSubTotalTaxTotalPriceField(Message email) throws MessagingException, IOException {
        String[] EXPECTED_SUBTOTAL_KEYWORD = {"Subtotal"};
        String[] EXPECTED_TAX_KEYWORD = {"Tax"};
        String[] EXPECTED_TOTAL_PRICE_KEYWORDS = {"Total", "Price"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(email, "td[class=body-text]");
        if (elements != null && elements.size() > 0) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, EXPECTED_SUBTOTAL_KEYWORD)) {
                    result.put("Subtotal", elements.get(i + 1).text().trim()); // get '$(price)' from next element of 'Subtotal'
                }
                if (StringHelper.containsIgnoreCase(text, EXPECTED_TAX_KEYWORD) && !StringHelper.containsIgnoreCase(text, "taxes")) {
                    result.put("Tax", elements.get(i + 1).text().trim()); // get '$(price)' from next element of 'Tax'
                }
                if (StringHelper.containsIgnoreCase(text, EXPECTED_TOTAL_PRICE_KEYWORDS)) {
                    result.put("Total Price", elements.get(i + 1).text().trim()); // get '$(price)' from next element of 'Total Price'
                }
            }
        }
        return result;
    }

    /**
     * Get the 'Subtotal' values from a specific e-mail for a given language enum.
     *
     * @param email Email message
     * @return String of the 'Subtotal'
     * @throws MessagingException if error occurs while closing connection.
     * @throws IOException if an I/O exception occurs.
     */
    private String getSubtotalPriceField(Message email)
            throws MessagingException, IOException {

        String[] EXPECTED_SUBTOTAL_KEYWORD = { I18NUtil.getMessage("subtotal") };
        Elements elements = getElementsMatchingSelector(email, "td[class=body-text]");
        if (elements != null && elements.size() > 0) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, EXPECTED_SUBTOTAL_KEYWORD)) {
                    return elements.get(i + 1).text().trim(); // get '$(price)' from next element of 'Subtotal'
                }
            }
        }
        return null;
    }

    /**
     * Get 'Subtotal' from an order confirmation e-mail given the language enum.
     *
     * @param email Email to get the 'Subtotal' from
     * @return The 'Subtotal' returned as a String
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public String getSubtotalField(Message email) throws IOException, MessagingException {
        return getSubtotalPriceField(email);
    }

    /**
     * Get 'Tax' from an order confirmation e-mail for the current locale.
     *
     * @param email Email to get the 'Tax' from
     * @return The 'Tax' returned as a String
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public String getTaxField(Message email) throws MessagingException, IOException {
        String[] EXPECTED_SUBTOTAL_KEYWORD = { I18NUtil.getMessage("tax") };
        Elements elements = getElementsMatchingSelector(email, "td[class=body-text]");
        if (elements != null && elements.size() > 0) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, EXPECTED_SUBTOTAL_KEYWORD)) {
                    return StringHelper.normalizeNumberString(elements.get(i + 1).text().trim());
                }
            }
        }
        return null;
    }

    /**
     * Gets 'Total Price' from an order confirmation e-mail.
     *
     * @param email Email to get the 'Total Price' from
     * @return The 'Total Price' returned as a String
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public String getTotalPriceField(Message email) throws MessagingException, IOException {
        String[] EXPECTED_SUBTOTAL_KEYWORD = { I18NUtil.getMessage("totalPrice") };
        Elements elements = getElementsMatchingSelector(email, "td[class=body-text]");
        if (elements != null && elements.size() > 0) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, EXPECTED_SUBTOTAL_KEYWORD)) {
                    return StringHelper.normalizeNumberString(elements.get(i + 1).text().trim());
                }
            }
        }
        return null;
    }

    /**
     * Checks to see if the redemption instructions are localized for a given language.
     *
     * @param message The message to check
     * @return true if the redemption instructions are localized for a given language,
     * false otherwise
     */
    public boolean verifyRedemptionInstructions(Message message) throws IOException, MessagingException {
        String[] EXPECTED_MY_GAMES_KEYWORD = { I18NUtil.getMessage("myGames") };
        Elements elements = getElementsMatchingSelector(message, "td[class=body-text]", EXPECTED_MY_GAMES_KEYWORD);
        if (elements != null && elements.size() > 0) {
            for (int i = 0; i < elements.size(); i++) {
                String text = elements.get(i).text();
                if (StringHelper.containsIgnoreCase(text, EXPECTED_MY_GAMES_KEYWORD)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Get all links from the email footer.
     *
     * @return {@link Map} of all the links shown in the email footer
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getAllLinksFieldInFooter() throws MessagingException, IOException {
        String[] EXPECTED_TERMS_OF_SALE_KEYWORDS = {"Terms", "of", "Sale"};
        String[] EXPECTED_LEGAL_NOTICE_KEYWORDS = {"Legal", "Notice"};
        String[] EXPECTED_PRIVACY_POLICY_KEYWORDS = {"Privacy", "Policy"};
        String[] EXPECTED_USER_AGREEMENT_KEYWORDS = {"User", "Agreement"};
        String[] EXPECTED_LEGAL_KEYWORDS = {"Legal"};
        String[] EXPECTED_FOOTER_KEYWORDS = {"EA", "is", "subject"};
        String[] EXPECTED_ORIGIN_GREAT_GAME_GUARANTEE_KEYWORDS = {"Origin", "Great", "Game", "Guarantee"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestGiftSentEmail(), "td[align=center]", EXPECTED_FOOTER_KEYWORDS);
        if (elements != null && elements.size() > 0) {
            elements = elements.get(elements.size() - 1).getAllElements(); // get footer section only
            for (int i = 1; i < elements.size(); i++) {
                String text = elements.get(i).text();
                String url = elements.get(i).attr("href").trim();
                if (!url.isEmpty()) {
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_TERMS_OF_SALE_KEYWORDS)) {
                        result.put("Terms of Sale", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_LEGAL_NOTICE_KEYWORDS)) {
                        result.put("Legal Notice", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_PRIVACY_POLICY_KEYWORDS)) {
                        result.put("Privacy Policy", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_USER_AGREEMENT_KEYWORDS)) {
                        result.put("User Agreement", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_LEGAL_KEYWORDS)) {
                        result.put("Legal", elements.get(i).attr("href").trim());
                    }
                }
            }
        }
        return result;
    }

    /**
     * Get all field values for latest 'Gift Sent' email.
     *
     * @return {@link Map} of all the retrieved fields in the latest 'Gift Sent' email
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Map<String, String> getLatestGiftSentEmailFields() throws IOException, MessagingException {
        Map<String, String> finalResults = new LinkedHashMap<>();
        finalResults.putAll(getOrderNumberField());
        finalResults.putAll(getOrderDateField());
        finalResults.putAll(getGiftSenderEmailField());
        finalResults.putAll(getGiftSentToMessageField());
        finalResults.putAll(getOrderSummaryTextField());
        finalResults.putAll(getEntitlementSentField());
        finalResults.putAll(getFooterField());
        finalResults.putAll(getSubTotalTaxTotalPriceField());
        finalResults.putAll(getAllLinksFieldInFooter());
        return finalResults;
    }

    /**
     * Get all links from the new OA subscription email footer.
     *
     * @return {@link Map} of all the links shown in the new OA subscription email footer
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private Map<String, String> getAllLinksFieldInNewOASubscriptionFooter() throws MessagingException, IOException {
        String[] EXPECTED_HELP_KEYWORDS = {"help"};
        String[] EXPECTED_TERMS_OF_SALE_KEYWORDS = {"Terms", "of", "Sale"};
        String[] EXPECTED_ORIGIN_ACCESS_TERMS_AND_CONDITIONS_KEYWORDS = {"Origin", "Access", "Terms", "and", "Conditions"};
        String[] EXPECTED_PRIVACY_POLICY_KEYWORDS = {"Privacy", "Policy"};
        String[] EXPECTED_USER_AGREEMENT_KEYWORDS = {"User", "Agreement"};
        String[] EXPECTED_LEGAL_KEYWORDS = {"Legal"};
        Map<String, String> result = new LinkedHashMap<>();
        Elements elements = getElementsMatchingSelector(getLatestNewOAMembershipEmail(), "td[align=center]>a");
        if (elements != null && elements.size() > 0) {
            for (int i = 1; i < elements.size(); i++) {
                String text = elements.get(i).text();
                String url = elements.get(i).attr("href").trim();
                if (!url.isEmpty()) {
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_HELP_KEYWORDS)) {
                        result.put("help", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_TERMS_OF_SALE_KEYWORDS)) {
                        result.put("Terms of Sale", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_ORIGIN_ACCESS_TERMS_AND_CONDITIONS_KEYWORDS)) {
                        result.put("Terms and Conditions", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_PRIVACY_POLICY_KEYWORDS)) {
                        result.put("Privacy Policy", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_USER_AGREEMENT_KEYWORDS)) {
                        result.put("User Agreement", elements.get(i).attr("href").trim());
                    }
                    if (StringHelper.containsIgnoreCase(text, EXPECTED_LEGAL_KEYWORDS)) {
                        result.put("Legal", elements.get(i).attr("href").trim());
                    }
                }
            }
        }
        return result;
    }


    /**
     * Get all field values for latest 'OA Membership starts now' email.
     *
     * @return {@link Map} of all the retrieved fields in the latest 'OA Membership starts now' email
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public Map<String, String> getLatestOANewMembershipEmailFields() throws IOException, MessagingException {
        Map<String, String> finalResults = new LinkedHashMap<>();
        finalResults.putAll(getSubjectLineField());
        finalResults.putAll(getSellerAddressField());
        finalResults.putAll(getEAIdUsernameField());
        finalResults.putAll(getPaymentMethodField());
        finalResults.putAll(getSignInToOriginField());
        finalResults.putAll(getDownloadItHereField());
        finalResults.putAll(getEAAccountField());
        finalResults.putAll(getAllLinksFieldInNewOASubscriptionFooter());
        return finalResults;
    }

    /**
     * From a given email message, retrieve element(s) with specified selector.
     *
     * @param message Email message
     * @param selector Selector (CSS style) to locate the element
     * @return Element(s) with the specified selector, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private static @Nullable
    Elements getElementsMatchingSelector(Message message, String selector) throws MessagingException, IOException {
        String contentType = message.getContentType();
        if (contentType.contains("TEXT/HTML")) {
            Object content = message.getContent();
            if (content != null) {
                Document doc = Jsoup.parse(content.toString());
                Elements elements = doc.select(selector);
                return elements;
            }
        }
        return null;
    }

    /**
     * From a given email message, retrieve element(s) with specified selector which text
     * matches the keywords.
     *
     * @param message Email message
     * @param selector Selector (CSS style) to locate the element
     * @param keywords Expected keywords of the element's text
     * @return Elements with specified selector which text has the matching
     * keywords, or null if not found
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    private static @Nullable
    Elements getElementsMatchingSelector(Message message, String selector, String[] keywords) throws MessagingException, IOException {
        Elements results = new Elements();
        String contentType = message.getContentType();
        if (contentType.contains("TEXT/HTML")) {
            Object content = message.getContent();
            if (content != null) {
                Document doc = Jsoup.parse(content.toString());
                Elements elements = doc.select(selector);
                for (Element element : elements) {
                    if (StringHelper.containsIgnoreCase(element.text(), keywords)) {
                        results.add(element);
                    }
                }
            }
        }
        return results;
    }

    /**
     * Gets the HTML from a given message.
     *
     * @param message The message to get the HTML of
     * @return The HTML as a String
     *
     * @throws MessagingException If an error occurs while closing connection.
     * @throws IOException If an I/O exception occurs.
     */
    public String getEmailHTML(Message message) throws MessagingException, IOException {
        try {
            String contentType = message.getContentType();
            if (contentType.contains("TEXT/HTML")) {
                Object content = message.getContent();
                if (content != null) {
                    // Since content is html content present in string parse it using JSoup
                    Document doc = Jsoup.parse(content.toString());
                    return doc.toString();
                }
            }
        } catch (IOException | MessagingException e) {
            _log.error("Not able to get the message's HTML.");
            throw new RuntimeException(e);
        }
        return null;
    }

    /**
     * Save message's HTML into a HTML file (currently saves in vertex directory as
     * a HTML file named 'message.html').
     *
     * @param message The message to get the HTML of
     */
    public boolean saveMessageAsHTML(Message message, OriginClient client) throws IOException, MessagingException {
        String nodeFullURL = client.getNodeURL();
        if(null != nodeFullURL) {
            try {
                String e = nodeFullURL + "/file/write?path=C:/ProgramData/Origin/Logs/message.html";
                String res = RestfulHelper.post(e, getEmailHTML(message));
                if (res.equals("ok")) {
                    return true;
                }
            } catch (URISyntaxException | IOException var6) {
                _log.error(var6);
            }
        }
        return false;
    }

    /**
     * Verify 'Subtotal', 'Taxes', and 'Total Price' in a given confirmation email matches given value for Canada and US.
     * Otherwise, for other countries, just verify 'Subtotal' matches given value.
     *
     * @param email The email to check
     * @param subtotal The expected 'Subtotal' value
     * @param tax The expected 'Tax' value
     * @param total The expected 'Total Price'
     * @return true if match, false otherwise
     */
    public boolean verifyPricesCorrect(Message email, String subtotal, String tax, String total) throws IOException, MessagingException {
        String subtotalFromEmail = StringHelper.normalizeNumberString(getSubtotalField(email));
        String currentCountry = I18NUtil.getLocale().getCountry();
        System.out.println("SUBTOTAL FROM EMAIL FOR " + currentCountry + ": " + subtotalFromEmail);
        System.out.println("SUBTOTAL FOR " + currentCountry + ": " + subtotal);
        if (currentCountry.equals("CA") || currentCountry.equals("US")) {
            String priceFromEmail = getTotalPriceField(email);
            String taxFromEmail = getTaxField(email);
            return StringHelper.containsIgnoreCase(subtotalFromEmail, subtotal) &&
                    StringHelper.containsIgnoreCase(priceFromEmail, total) && StringHelper.containsIgnoreCase(taxFromEmail, tax);
        } else {
            return StringHelper.containsIgnoreCase(subtotalFromEmail, subtotal);
        }
    }

    /**
     * Wait and get the latest email with the given subject.
     *
     * @param subject The subject name
     * @return The latest message with the given subject
     * @throws IOException
     * @throws MessagingException
     */
    public Message waitAndGetEmailWithSubject(String subject) throws IOException, MessagingException {
        // EADPs server is inconsistent when receiving e-mails, especially on Integration
        // so this will check every minute up to 7 minutes to see if the correct e-mail
        // has come in and will break early when found.
        for (int i = 0; i < 7; ++i) {
            Waits.sleepMinutes(1);
            Message orderConfirmation = getLatestEmailWithMatchingSubject(subject);
            if (orderConfirmation != null) {
                return orderConfirmation;
            }
        }
        return null;
    }
}