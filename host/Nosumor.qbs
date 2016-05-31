import qbs

QtGuiApplication {
    Depends {name: "Qt.widgets"}
    consoleApplication: false
    cpp.cxxLanguageVersion: "c++11"
    cpp.dynamicLibraries: ["setupapi"]

    files: [
        "*.cpp",
        "*.c",
        "*.h",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}
