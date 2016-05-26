import qbs

Project {
    property string driversDir: "../embedded-drivers"
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
            cpp.includePaths: ["src"]

            Properties {
                condition: qbs.buildVariant == "release"
                cpp.optimization: "small"
            }
        }

        files: [
            "src/stm32f1xx_hal_conf.h",
        ]
    }

    Product {
        Depends {name: "cpp"}
        Depends {name: "CMSIS"}
        type: "application"
        //Depends {name: "configurations"}
        cpp.linkerScripts: ["STM32F103C8_SRAM.ld"]
        cpp.includePaths: ["src"]

        files: [
            "src/keyboard.*",
            "src/usb.*",
            "src/dma.*",
            "src/usb_def.*",
            "src/usb_setup.*",
            "src/usb_desc.*",
            "src/debug.*",
            "src/main.c",
            "src/startup_stm32f103xb.s",
            "src/system_stm32f1xx.c",
        ]

        Group {
            name: "USB"
            condition: false
            cpp.optimization: "small"
            cpp.commonCompilerFlags: outer.concat([
                "-Wno-unused-parameter",
                "-Wno-pointer-sign",
            ])
            files: [
                "src/usb_device.h",
                "src/usbd_conf.h",
                "src/usbd_custom_hid_if.h",
                "src/usbd_desc.h",
                "src/usb_device.c",
                "src/usbd_conf.c",
                "src/usbd_custom_hid_if.c",
                "src/usbd_desc.c",
            ]
        }
    }
}
