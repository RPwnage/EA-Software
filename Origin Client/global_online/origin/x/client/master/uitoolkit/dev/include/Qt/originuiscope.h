//
// 1) do NOT put #ifdef in here because this needs to be defined in multiple files
// 2) should not need a namespace because it will be included inside of namespaces
//

    /// \brief Where in the world is the element?
    /// Normal - Window is outside of the OIG. Window has no special treatment
    /// OIG - Window is in of the OIG. Element most likely will be modified.
    enum WindowContext
    {
          Normal
        , OIG
    };