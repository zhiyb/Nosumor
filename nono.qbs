import qbs

Project {
    property string driversDir: "../Drivers"
    property string device: "STM32F103C8T6"
    property string usb_device_class: "HID"
    property bool build_CMSIS_DSP: false

    references: [
        driversDir + "/CMSIS/cmsis.qbs",
        driversDir + "/STM32F1xx_HAL_Driver",
        driversDir + "/Middlewares/ST/STM32_USB_Device_Library",
    ]

    Product {
        name: "configurations"

        Export {
            Depends {name: "cpp"}
            cpp.includePaths: ["Inc"]

            Properties {
                condition: qbs.buildVariant == "release"
                cpp.optimization: "small"
            }
        }

        files: [
            "Inc/stm32f1xx_hal_conf.h",
        ]
    }

    CppApplication {
        Depends {name: "CMSIS"}
        Depends {name: "STM32-HAL"}
        Depends {name: "STM32-USB-DEVICE"}
        Depends {name: "configurations"}
        cpp.linkerScripts: ["STM32F103C8_FLASH.ld"]
        cpp.includePaths: ["Inc"]

        files: [
            "Inc/debug.h",
            "Inc/keyboard.h",
            "Inc/mxconstants.h",
            "Inc/stm32f1xx_it.h",
            "STM32F103C8_FLASH.ld",
            "Src/debug.c",
            "Src/main.c",
            "Src/stm32f1xx_hal_msp.c",
            "Src/stm32f1xx_it.c",
        ]

        Group {
            name: "USB"
            condition: true
            cpp.commonCompilerFlags: outer.concat([
                "-Wno-unused-parameter",
                "-Wno-pointer-sign",
            ])
            files: [
                "Inc/usb_device.h",
                "Inc/usbd_conf.h",
                "Inc/usbd_desc.h",
                "Src/usb_device.c",
                "Src/usbd_conf.c",
                "Src/usbd_desc.c",
            ]
        }
    }
}
