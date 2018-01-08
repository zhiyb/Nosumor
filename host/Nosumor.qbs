import qbs

Project {
    minimumQbsVersion: "1.7.1"

    references: [
        "console",
        "qt",
        "plugins",
    ]

    Product {
        Export {
            Depends {name: "cpp"}
            Depends {name: "hidapi"}
            cpp.commonCompilerFlags: ["-mno-ms-bitfields"]
            cpp.includePaths: [
                "include",
                "../device",
            ]
        }

        files: [
            "include/dev_defs.h",
            "../device/vendor_defs.h",
        ]
    }

    StaticLibrary {
        name: "hidapi"
        Depends {name: "cpp"}
        cpp.includePaths: ["hidapi/hidapi"]
        cpp.optimization: "fast"

        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.dynamicLibraries: ["udev"]
        }

        Properties {
            condition: qbs.targetOS.contains("windows")
            cpp.dynamicLibraries: ["setupapi"]
        }

        Export {
            Depends {name: "cpp"}
            cpp.includePaths: ["hidapi/hidapi"]
        }

        files: ["hidapi/hidapi/hidapi.h"]

        Group {
            name: "Linux"
            condition: qbs.targetOS.contains("linux")
            files: ["hidapi/linux/hid.c"]
        }

        Group {
            name: "Windows"
            condition: qbs.targetOS.contains("windows")
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
            Properties {
                condition: qbs.targetOS.contains("linux")
                cpp.dynamicLibraries: ["pthread"]
            }
        }

        files: ["spdlog/include/**"]
    }
}
