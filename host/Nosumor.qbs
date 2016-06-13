import qbs

QtGuiApplication {
    Depends {name: "Qt.widgets"}
    consoleApplication: false
    cpp.cxxLanguageVersion: "c++11"
    cpp.dynamicLibraries: ["setupapi"]
    cpp.includePaths: ["../device/usb"]

    files: [
        "*.cpp",
        "*.h",
        "resources.qrc",
    ]

    Group {
        name: "hid.c"
        cpp.commonCompilerFlags: ["-Wno-unused-parameter"]
        files: ["hid.c"]
    }

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}
