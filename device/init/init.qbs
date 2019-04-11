import qbs

StaticLibrary {
    // init             System control module
    //                      Always init/start first
    //
    // Module messages:
    // init                 System initialisation
    // start                Start system peripherals
    //                          Set the first available UART as stdio
    // active               Activate IRQs
    // mco1                 External clock output GPIO control
    //                          data: uint32_t enable;  // Enable/disable MCO1 GPIO
    // tick.get             System tick counter
    //                          * Direct type casting
    //                          uint32_t tick;          // Tick counter value
    // tick.delay           Delay for some ticks
    //                          * Direct type casting
    //                          uint32_t tick;          // Number of ticks to wait

    Depends {name: "core"}

    Export {
        Depends {name: "cpp"}
        Depends {name: "core"}
        Parameters {cpp.linkWholeArchive: true}
        cpp.includePaths: ["."]
    }

    files: [
        "init.c",
        "pvd.c",
        "pvd.h",
        "systick.c",
        "systick.h",
    ]
}
