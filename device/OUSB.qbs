import qbs

Project {
    name: "OUSB"
    minimumQbsVersion: "1.7.1"

    property string device: "STM32F722"
    property bool cmsis_dsp: false
    property bool bootloader: false
    property int hwver: 0x0100
    property string hwver_str: ("0000" + hwver.toString(16)).slice(-4)

    references: [
        "CMSIS",
        "core",
        "init",
        "uart",
        "led",
        "keyboard",
        //"api",
    ]

    CppApplication {
        name: project.name
        type: ["application", "hex", "size"]
        Depends {name: "core"}
        Depends {name: "init"}
        Depends {name: "uart"}
        Depends {name: "led"}
        Depends {name: "keyboard"}
        //Depends {name: "api"}
        Depends {name: "gcc-none"}

        files: [
            "main.c",
        ]

        Group {
            name: "Linker scripts"
            files: [
                "ld/1_common.ld",
                "ld/2_lists.ld",
            ]
        }

        Group {
            name: "Linker script for AXI"
            condition: qbs.buildVariant === "debug"
            files: "ld/0_STM32F722RETx_FLASH_AXIM.ld"
        }

        Group {
            name: "Linker script for ITCM"
            condition: qbs.buildVariant === "release"
            files: "ld/0_STM32F722RETx_FLASH_ITCM.ld"
        }

        Group {     // Properties for the produced executable
            fileTagsFilter: ["application", "hex", "bin"]
            qbs.install: true
        }
    }
}
