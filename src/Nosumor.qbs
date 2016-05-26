import qbs

Project {
    property string driversDir: "../../embedded-drivers"
    property string device: "STM32F103C8T6"
    property string usb_device_class: "" //"CustomHID"
    property bool build_CMSIS_DSP: false

    references: [
        driversDir + "/CMSIS/cmsis.qbs",
        driversDir + "/STM32F1xx_HAL_Driver",
        driversDir + "/Middlewares/ST/STM32_USB_Device_Library",
    ]

    Product {
        name: "configurations"
        //condition: false

        Export {
            Depends {name: "cpp"}
            cpp.includePaths: ["bak"]

            Properties {
                condition: qbs.buildVariant == "release"
                cpp.optimization: "small"
            }
        }

        files: [
            "bak/stm32f1xx_hal_conf.h",
        ]
    }

    Product {
        Depends {name: "cpp"}
        Depends {name: "CMSIS"}
        type: "application"
        //Depends {name: "configurations"}
        cpp.linkerScripts: ["STM32F103C8_SRAM.ld"]
        cpp.includePaths: [".", "usb"]

        files: [
            "*.h",
            "*.c",
            "*.s",
        ]

        Group {
            name: "USB"
            files: ["usb/**"]
        }

        Group {
            name: "bak"
            condition: false
            cpp.optimization: "small"
            cpp.commonCompilerFlags: outer.concat([
                "-Wno-unused-parameter",
                "-Wno-pointer-sign",
            ])
            files: ["bak/**"]
        }
    }
}
