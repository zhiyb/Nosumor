import qbs

StaticLibrary {
    // led              (RGB) LED peripheral module
    //
    // Module messages:
    // init             Initialisation
    // start            Set LEDs with default colours
    // info             Infomation about available LEDs
    //                      TODO

    Depends {name: "core"}

    Export {
        Parameters {cpp.linkWholeArchive: true}
    }

    files: [
        "led.c",
    ]
}
