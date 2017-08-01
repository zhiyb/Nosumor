import qbs

Project {
    minimumQbsVersion: "1.7.1"

    references: [
        "console"
    ]

    Product {
        Export {
            Depends {name: "cpp"}
            Depends {name: "hidapi"}
            cpp.includePaths: [
                "include",
                "../device",
            ]
        }

        files: [
            "include/dev_defs.h"
        ]
    }

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

    Product {
        name: "spdlog"
        Export {
            Depends {name: "cpp"}
            cpp.cxxLanguageVersion: "c++11"
            cpp.includePaths: ["spdlog/include"]
            cpp.defines: ["SPDLOG_WCHAR_TO_UTF8_SUPPORT"]
        }

        files: ["spdlog/include/**"]
    }
}
