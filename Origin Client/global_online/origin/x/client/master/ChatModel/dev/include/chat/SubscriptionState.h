#ifndef _CHATMODEL_SUBSCRIPTIONSTATE_H
#define _CHATMODEL_SUBSCRIPTIONSTATE_H

namespace Origin
{
namespace Chat
{
    /// 
    /// Represents a subscription state between a roster contact and the connected user
    ///
    /// Subscription states are complex topic. They are described in detail in RFC 6121 Appendix A.
    ///
    class SubscriptionState
    {
    public:
        /// 
        /// Direction of the current presence subscription
        ///
        /// Defined in RFC 6121 section 2.1.2.5
        ///
        /// \sa Chat::SubscriptionState::isContactSubscribedToCurrentUser
        /// \sa Chat::SubscriptionState::isCurrentUserSubscribedToContact
        ////
        enum SubscriptionDirection
        {
            ///
            /// No presence subscription exists
            ///
            DirectionNone,

            ///
            /// The user is subscribed to the contact's presence; the contact is not subscribed to the user's presence
            ///
            DirectionTo,
            
            ///
            /// The contact is subscribed to the user's presence; the user is not subscribed to the contact's presence
            ///
            DirectionFrom,

            ///
            /// The user and contact are mutually subscribed to each other's presence
            ///
            DirectionBoth
        };

        ///
        /// Creates a null subscription state
        ///
        /// \sa isNull()
        ///
        SubscriptionState() : mNull(true) {}

        /// 
        /// Creates a new subscription state
        ///
        /// \param  direction      Direction of the subscription
        /// \param  pendingContact True if this subscription is pending contact approval
        /// \param  pendingUser    True if this subscription is pending current user approval
        ///
        SubscriptionState(SubscriptionDirection direction, bool pendingContact, bool pendingUser)
            : mNull(false), mDirection(direction), mPendingContact(pendingContact), mPendingUser(pendingUser)
        {
        }

        ///
        /// Returns if this is a null subscription state
        ///
        bool isNull() const
        {
            return mNull;
        }

        ///
        /// Returns the direction of this subscription
        ///
        /// It might be easier to use isContactSubscribedToCurrentUser() or isCurrentUserSubscribedToContact()
        ///
        SubscriptionDirection direction() const
        {
            return mDirection;
        }

        ///
        /// Returns if this is pending approval from the other party
        ///
        bool isPendingContactApproval() const
        {
            return mPendingContact;
        }
        
        ///
        /// Returns if this is pending approval from the current user
        ///
        bool isPendingCurrentUserApproval() const
        {
            return mPendingUser;
        }

        /// 
        /// Returns if this contact is subscribed to our presence
        ///
        bool isContactSubscribedToCurrentUser() const
        {
            return (direction() == DirectionTo) || (direction() == DirectionBoth);
        }

        ///
        /// Returns if we're subscribed to the contact's presence
        ///
        bool isCurrentUserSubscribedToContact() const
        {
            return (direction() == DirectionFrom) || (direction() == DirectionBoth);
        }

        ///
        /// Compares this SubscriptionState for exact equality
        ///
        bool operator==(const SubscriptionState &other) const
        {
            return (direction() == other.direction()) && 
                (isPendingContactApproval() == other.isPendingContactApproval()) &&
                (isPendingCurrentUserApproval() == other.isPendingCurrentUserApproval()) &&
                (isNull() == other.isNull());
        }
        
        ///
        /// Compares this SubscriptionState for inequality
        ///
        bool operator!=(const SubscriptionState &other) const
        {
            return !(*this == other);
        }

        ///
        /// Duplicates this intance with a specific pending current user value
        ///
        SubscriptionState withPendingCurrentUser(bool pendingUser) const
        {
            if (isNull())
            {
                return SubscriptionState();
            }
            else
            {
                return SubscriptionState(direction(), isPendingContactApproval(), pendingUser); 
            }
        }
    
    private:
        bool mNull;
        SubscriptionDirection mDirection;
        bool mPendingContact;
        bool mPendingUser;
    };
}
}

#endif
