import qbs

StaticLibrary {
    // keyboard         Keyboard peripheral module
    //
    // Module messages:
    // init             Initialisation
    // start            Start listening for key presses
    // info             Infomation about available keys
    //                      uint32_t keys;          // Number of keys
    //                      uint32_t masks[keys];   // Status bitmap masks
    //                      char *   names[keys];   // Key names
    // status           Current key status bitmap
    //                      * Direct type casting
    //                      uint32_t status;        // Status bitmap
    // handler.install  Install key press event handler
    //                      * Direct type casting
    //                      data: void handler(uint32_t status);
    //                          status: Status bitmap of all keys

    Depends {name: "core"}
    Depends {name: "init"}

    Export {
        Depends {name: "cpp"}
        Depends {name: "core"}
        Parameters {cpp.linkWholeArchive: true}
        cpp.includePaths: ["."]
    }

    Properties {
        cpp.optimization: "fast"
    }

    files: [
        "keyboard.c",
        "keyboard.h",
    ]
}
