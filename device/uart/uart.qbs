import qbs

StaticLibrary {
    // uart             UART peripheral module
    //
    // Module messages:
    // init             Initialisation
    // config           Configure UART baudrate, and start UART
    //                      data: uint32_t baud;    // Baudrate
    // stdio            Set this UART as stdio

    Depends {name: "core"}

    Export {
        Parameters {cpp.linkWholeArchive: true}
    }

    Properties {
        cpp.optimization: "fast"
    }

    files: [
        "uart.c",
    ]
}
