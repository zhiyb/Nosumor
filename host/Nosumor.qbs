import qbs

Project {
    minimumQbsVersion: "1.7.1"

    StaticLibrary {
        name: "hidapi"
        Depends {name: "cpp"}
        cpp.includePaths: ["hidapi/hidapi"]
        cpp.optimization: "fast"
        cpp.commonCompilerFlags: ["-Wno-unused-parameter"]
        cpp.dynamicLibraries: ["setupapi"]

        files: ["hidapi/hidapi/hidapi.h"]

        Export {
            Depends {name: "cpp"}
            cpp.includePaths: ["hidapi/hidapi"]
        }

        Group {
            name: "Linux"
            condition: qbs.targetOS == "linux"
            files: ["hidapi/linux/hid.c"]
        }

        Group {
            name: "Windows"
            condition: qbs.targetOS == "windows"
            files: ["hidapi/windows/hid.c"]
        }
    }

    CppApplication {
        consoleApplication: true
        Depends {name: "hidapi"}

        files: [
            "hid_defs.h",
            "main.cpp",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: product.type
            qbs.install: true
        }
    }
}
