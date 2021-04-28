ArsonComponent
--------------

The purpose of this component is to provide a quick skeleton for creating
components.

It will be broken down into a few sections.

    1. Creating your component
    2. Adding commands to the BlazeTool (bt)
    3. Customizaing your component

1. Creating your Component
--------------------------
You can create your component in 2 ways: from scratch or from a copy of the
arson component.

Before you begin, you will need to decide on a few things.

- <xxx>: The unique 3-digit HEX number identifying your component.
- <yourcomponent>: The name of your component (use only lower case for files but
  camel case for names).
- <yourtypes>: The name of the .tdf file used to define your types. (use only
  lower case for files but camel case for names).
- <yourname>: The soft-name for your component.  It can be the same as
  <yourcomponent>.  (e.g. Arson and arson, or ClientSets and clientsets
  respectively).

1.1 Creating a Skeleton Component from Arson Component

Convention(s):
    - $/path = /trunk/component/<yourcomponent>/path.

Steps:
    1) Copy the /trunk/arsoncomponent directory to /trunk/<yourcomponent> directory.

    2) Edit $/gen/arson.tdf.
        - Rename arson.tdf to <yourtypes>.tdf.
        - Change "namespace Blaze::ArsonComponent" to "namespace Blaze::<YourComponent>".

    3) Edit $/gen/arsoncomponent.rpc.
        - Rename arsoncomponent.rpc to <yourcomponent>component.rpc.
        - Edit "component ArsonSlave {}" from:
        ---------------
        component ArsonComponentSlave
        {
            id = 0x0f00;
            namespace = Blaze::ArsonComponent;
            name = arson;
            clientservicename = Arson;
            impl = ArsonComponentSlaveImpl;
            implheader = "arsoncomponentslaveimpl.h";

            include "arson.h";

            include "pokeslave_command.h";
            include "pokemaster_command.h";

            methods
            {
                ArsonResponse pokeSlave(ArsonRequest args) { id = 1; }
            }
        }
        ---------------
        to: (where <xxx> is a HEX number for your component).
        ---------------
        component <YourComponent>Slave
        {
            id = 0x0<xxx>;
            namespace = Blaze::<YourComponent>;
            name = <yourname>;
            clientservicename = <YourComponent>;
            impl = <YourComponent>SlaveImpl;
            implheader = "<yourcomponent>slaveimpl.h";

            include "<yourtypes>.h";

            include "pokeslave_command.h";
            include "pokemaster_command.h";

            methods
            {
                ArsonResponse pokeSlave(ArsonRequest args) { id = 1; }
            }
        }
        ---------------
        - Edit "component ArsonComponentMaster {}" from:
        ---------------
        component ArsonComponentMaster
        {
            id = 0x1f00;
            namespace = Blaze::ArsonComponent;
            name = arson_master;
            impl = ArsonComponentMasterImpl;
            implheader = "arsoncomponentmasterimpl.h";

            include "arson.h";

            include "pokeslave_command.h";
            include "pokemaster_command.h";

            methods
            {
                ArsonResponse pokeMaster(ArsonRequest args) { id = 2; }
            }
        }
        ---------------
        to: (where <xxx> is a HEX number for your component).
        ---------------
        component <YourComponent>Master
        {
            id = 0x1<xxx>;
            namespace = Blaze::<YourComponent>;
            name = <yourname>_master;
            impl = <YourComponent>MasterImpl;
            implheader = "<yourcomponent>masterimpl.h";

            include "<yourtypes>.h";

            include "pokeslave_command.h";
            include "pokemaster_command.h";

            methods
            {
                ArsonResponse pokeMaster(ArsonRequest args) { id = 2; }
            }
        }
        ---------------

    4) Edit /trunk/blazerpccomponent.rpc.
        - Add a block for your component:
        ---------------
        ifbld arsoncomponent
            include "arsoncomponent/gen/arsoncomponentcomponent.rpc";
        endbld
        ---------------
        to:
        ---------------
        ifbld arsoncomponent
            include "arsoncomponent/gen/arsoncomponentcomponent.rpc";
        endbld
        ifbld <yourcomponent>
            include "<yourcomponent>/gen/<yourcomponent>component.rpc";
        endbld
        ---------------

    5) Edit $/[include|source]/arsoncomponent*impl.*
        - Rename arsoncomponent*impl.* to <yourcomponent>*impl.*.
        - In *.h:
            - Change the .h exclusive include defines.  (Replace _DUMMYCOMPONTENT_ with _<YOURCOMPONENT>_)
            - Change the namespace from ArsonComponent to <YourComponent>.
            - Change the Class names, Super class names and construction/destructor names (from ArsonComponent* to <YourComponent>*).
        - In *.cpp:
            - Change (include "arsoncomponent*impl.h") to (include "<yourcomponent>*impl.h").
            - Change the namespace from ArsonComponent to <YourComponent>.
            - Change the Class names, Super class names and construction/destructor names (from ArsonComponent* to <YourComponent>*).

    6) Edit pokeslave_command.*.
        - In *.h:
            - Change the .h exclusive include defines.  (Replace _DUMMYCOMPONTENT_ with _<YOURCOMPONENT>_)
            - Change (#include "arsoncomponentslaveimpl.h") to (#include "<yourcomponent>slaveimpl.h")
            - Change "namespace Blaze::ArsonComponent" to "namespace Blaze::<YourComponent>".
            - Change (ArsonComponentSlaveImpl* mComponent;) to (<YourComponent>SlaveImpl* mComponent;)
        - In *.cpp:
            - Change "namespace Blaze::ArsonComponent" to "namespace Blaze::<YourComponent>".
            - In the PokeSlaveCommand constructor, change "mComponent((ArsonComponentSlaveImpl*) componentImpl" to "mComponent((<YourComponent>SlaveImpl*) componentImpl".

    7) Edit pokemaster_command.*.
        - In *.h:
            - Change the .h exclusive include defines.  (Replace _DUMMYCOMPONTENT_ with _<YOURCOMPONENT>_)
            - Change (#include "arsoncomponentmasterimpl.h") to (#include "<yourcomponent>masterimpl.h")
            - Change "namespace Blaze::ArsonComponent" to "namespace Blaze::<YourComponent>".
            - Change (ArsonComponentMasterImpl* mComponent;) to (<YourComponent>MasterImpl* mComponent;)
        - In *.cpp:
            - Change "namespace Blaze::ArsonComponent" to "namespace Blaze::<YourComponent>".
            - In the PokeSlaveCommand constructor, change "mComponent((ArsonComponentMasterImpl*) componentImpl" to "mComponent((<YourComponent>MasterImpl*) componentImpl".

    8) Add your component to make.
        - Copy /trunk/make/lib/arsoncomponent.mak to <yourcomponent>.mak.
        - Open <yourcomponent>.mak and replace all "arsoncomponent" with "<yourcomponent>".
        - Open /trunk/blazerpccomponent.rpc and add your component(similar to arson) for the generation of rpc files.

    9) Edit server properties.
        - Edit /trunk/blazeserver/slave.properties.  Change:
        ---------------
        slavecomponents = [
          ...
          { id = arson },
          ...
        ]
        ---------------
        to:
        ---------------
        slavecomponents = [
          ...
          { id = arson },
          { id = <yourname> },
          ...
        ]
        ---------------
        (order does not matter).
        - Edit /trunk/blazeserver/master.properties.  Change:
        ---------------
        mastercomponents = [
          ...
          { id = arson_master },
          ...
        ]
        ---------------
        to:
        ---------------
        slavecomponents = [
          ...
          { id = arson_master },
          { id = <yourname>_master },
          ...
        ]
        ---------------
       (order does not matter).
        - Edit /trunk/blazeserver/coloc.properties.
            - Do the same changes as in slave.properties.
            - Do the same changes as in master.properties.

    10) Compile and run.  You should be able to poke your new skeleton component now.


1.2 Creating a Component from Scratch

    -- TODO


2. Adding commands to the BlazeTool (bt)
----------------------------------------
Probably the fastest/easiest way to add to BT is to copy from the
ArsonComponent commands.

Before you begin, you will need to decide on a few things (see Section 1 for more)

- <yourprefix>: The prefix command you would like to use to for your command set.

2.1 Copying from ArsonComponent

Convention(s):
    - $/path = /trunk/tools/bt/path.

Steps:
    1) In /trunk/tools/bt
        - copy include/arsoncomponentcommands.h to include/<yourcomponent>commands.h.
        - copy source/arsoncomponentcommands.cpp to source/<yourcomponent>commands.cpp.

    2) Edit $/<yourcomponent>commands.h
        - Change the .h exclusive include defines.  (Replace _DUMMYCOMPONTENT_ with _<YOURCOMPONENT>_)
        - Change "class ArsonComponentPokeSlaveCommand" to "class <YourComponent>PokeSlaveCommand" and all corresponding references.
        - Change "class ArsonComponentCommand" to "class <YourComponent>Command", and all constructors/destructors.
        - Edit description in "class <YourComponent>Command" if desired.

    3) Edit $/<yourcomponent>commands.cpp
        - Change "using namespace ArsonComponent" to "using namespace <YourComponent>".
        - Change "ArsonComponentPokeSlaveCommand" to "<YourComponent>PokeSlaveCommand".

    4) Edit $/main.cpp
        - Copy the following block:
        ----------------------
        #ifdef TARGET_arsoncomponent
        #include "arsoncomponentcommands.h"
        #endif
        ----------------------
        paste it below and change it to:
        ----------------------
        #ifdef TARGET_<yourcomponent>
        #include "<yourcomponent>commands.h"
        #endif
        ----------------------
        - Copy the following block:
        ----------------------
        #ifdef TARGET_arsoncomponent
            gCommands["arson"] = new ArsonComponentCommand();
        #endif
        ----------------------
        paste it below  and change it to:
        ----------------------
        #ifdef TARGET_<yourcomponent>
            gCommands["<yourprefix>"] = new <YourComponent>Command();
        #endif
        ----------------------

    5) Edit /trunk/make/bin/bt.mak
        - Copy the following block:
        ----------------------
        ifdef TARGET_arsoncomponent
            SRC += tools/bt/arsoncomponentcommands.cpp
            INCLUDES += $(GENDIR)/arsoncomponent/include arsoncomponent/include
        endif
        ----------------------
        paste it below and change it to:
        ----------------------
        ifdef TARGET_<yourcomponent>
            SRC += tools/bt/<yourcomponent>commands.cpp
            INCLUDES += $(GENDIR)/<yourcomponent>/include <yourcomponent>/include
        endif
        ----------------------
        - Add <yourcomponent> to the BINLIBS list.

    6) Compile and run.  You should have commands in bt to poke your new skeleton component now.


2.2 Adding to the command set


3. Customizaing your component
-------------------------------

-- TODO
