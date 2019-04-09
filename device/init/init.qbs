import qbs

StaticLibrary {
    // init             System control module
    //                      Always init/start first
    //
    // Module messages:
    // init                 System initialisation
    // start                Start system peripherals
    //                          Set the first available UART as stdio
    // mco1                 External clock output GPIO control
    //                          data: uint32_t enable;  // Enable/disable MCO1 GPIO
    // tick.get             System tick counter
    //                          * Direct type casting
    //                          uint32_t tick;          // Tick counter value
    // tick.delay           Delay for some ticks
    //                          * Direct type casting
    //                          uint32_t tick;          // Number of ticks to wait
    // tick.handler.install Install system tick event handler
    //                          * Direct type casting
    //                          data: void handler(uint32_t cnt);
    //                              cnt: Tick counter value

    Depends {name: "core"}

    Export {
        Parameters {cpp.linkWholeArchive: true}
    }

    files: [
        "init.c",
        "pvd.c",
        "pvd.h",
        "systick.c",
        "systick.h",
    ]
}
