import qbs

Project {
    property string driversDir: "../../embedded-drivers"
    property string device: "STM32F103C8T6"
    property bool cmsis_dsp: false

    references: [
        driversDir + "/CMSIS/cmsis.qbs",
    ]

    Product {
        type: "application"
        Depends {name: "cpp"}
        Depends {name: "CMSIS"}
        cpp.linkerScripts: ["STM32F103C8_SRAM.ld"]
        cpp.includePaths: [".", "usb"]

        Properties {
            condition: qbs.buildVariant == "debug"
            cpp.defines: ["DEBUG"]
        }

        Properties {
            condition: qbs.buildVariant == "release"
            cpp.optimization: "small"
        }

        files: [
            "*.h",
            "*.c",
            "*.s",
        ]

        Group {
            name: "USB"
            files: ["usb/**"]
        }
    }
}
