import qbs

Product {
    Depends {name: "cpp"}
    cpp.includePaths: ["."]

    Export {
        Depends {name: "cpp"}
        cpp.includePaths: ["."]
    }

    files: [
        "api_defs.h",
    ]
}
