ThirdPartyOptIn Component
--------------

The purpose of this component is to provide a way for the client to use the EADP Preference Center service by forwarding it through the blazeserver.
The requirements that forwarding it through the blazeserver will meet are:
    - No need for the client to send the user's email address, which is considered PII; the blazeserver will facilitate the lookup of the user's email address through the user's session.
    - Ability to store some arbitrary piece of information (an integer) on the blazeserver.
Design:
    - https://confluence.ea.com/display/nhl/Game+Modes+-+NHL+Opt-In
