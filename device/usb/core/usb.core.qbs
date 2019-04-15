import qbs

StaticLibrary {
    // usb              USB device peripheral module
    //
    // Module messages:
    // init             Initialisation
    // start            Start listening for key presses

    Depends {name: "core"}
    Depends {name: "init"}

    Export {
        Depends {name: "cpp"}
        Depends {name: "core"}
        Parameters {cpp.linkWholeArchive: true}
        cpp.includePaths: ["."]
    }

    files: [
        "usb.c",
        "usb_macros.h",
        "usb_irq.c",
        "usb_ep.h",
        "usb_ep.c",
        "usb_ep0.h",
        "usb_ep0.c",
    ]
}
