import qbs

Project {
    name: "Nosumor_" + hwver_str
    minimumQbsVersion: "1.7.1"

    property string device: "STM32F722"
    property bool cmsis_dsp: false
    property bool bootloader: false
    property int hwver: 0x0102
    property string hwver_str: ("0000" + hwver.toString(16)).slice(-4)

    references: [
        "CMSIS",
        "core.qbs",
    ]

    CppApplication {
        name: project.name
        type: ["application", "hex", "size"]
        Depends {name: "core"}
        Depends {name: "gcc-none"}

        files: [
            "main.c",
        ]

        Group {
            name: "Linker script for AXI"
            condition: qbs.buildVariant === "debug"
            files: "STM32F722RETx_FLASH_AXIM.ld"
        }

        Group {
            name: "Linker script for ITCM"
            condition: qbs.buildVariant === "release"
            files: "STM32F722RETx_FLASH_ITCM.ld"
        }

        Group {     // Properties for the produced executable
            fileTagsFilter: ["application", "hex", "bin"]
            qbs.install: true
        }
    }
}
