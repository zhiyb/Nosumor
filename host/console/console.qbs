import qbs

Project {
    minimumQbsVersion: "1.7.1"

    CppApplication {
        consoleApplication: true
        Depends {name: "Nosumor"}
        Depends {name: "spdlog"}

        files: [
            "logger.h",
            "main.cpp",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: product.type
            qbs.install: true
        }
    }
}
