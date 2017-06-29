import qbs

Project {
    property string device: "STM32F722"
    property bool cmsis_dsp: false

    minimumQbsVersion: "1.7.1"
    references: ["CMSIS"]

    CppApplication {
        cpp.includePaths: ["."]
        cpp.commonCompilerFlags: [
            "-Wno-unused-parameter",
            "-Wno-unused-function",
        ]
        Depends {name: "CMSIS"}

        Properties {
            condition: qbs.buildVariant == "debug"
            cpp.defines: ["DEBUG"]
        }

        Properties {
            condition: qbs.buildVariant == "release"
            cpp.optimization: "small"
        }

        files: [
            "STM32F722RETx_FLASH.ld",
            "peripheral/audio.c",
            "peripheral/audio.h",
            "clock.c",
            "clock.h",
            "debug.h",
            "escape.h",
            "fio.c",
            "fio.h",
            "macros.h",
            "main.c",
            "peripheral/uart.c",
            "peripheral/uart.h",
            "startup_stm32f722xx.S",
            "syscalls.c",
            "system_stm32f7xx.c",
            "systick.c",
            "systick.h",
            "usb/usb.c",
            "usb/usb.h",
            "usb/usb_desc.c",
            "usb/usb_desc.h",
            "usb/usb_ep.c",
            "usb/usb_ep.h",
            "usb/usb_ep0.c",
            "usb/usb_ep0.h",
            "usb/usb_irq.c",
            "usb/usb_irq.h",
            "usb/usb_macros.h",
            "usb/usb_ram.c",
            "usb/usb_ram.h",
            "usb/usb_setup.c",
            "usb/usb_setup.h",
        ]

        FileTagger {
            patterns: "*.ld"
            fileTags: ["linkerscript"]
        }
    }
}
