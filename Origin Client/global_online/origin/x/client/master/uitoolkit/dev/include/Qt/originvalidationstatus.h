//
// 1) do NOT put #ifdef in here because this needs to be defined in multiple files
// 2) should not need a namespace because it will be included inside of namespaces
//

    /// \brief Enum that controls what the validation setting should be
    enum OriginValidationStatus
    {
        Normal,		///< The item is in its normal/natural state.
        Validating, ///< The item is in the process of validating itself.
        Valid,      ///< The item is valid.
        Invalid     ///< The item is invalid.
    };
