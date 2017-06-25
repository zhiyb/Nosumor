import qbs

Project {
    property string device: "STM32F722"
    property bool cmsis_dsp: false

    minimumQbsVersion: "1.7.1"
    references: ["CMSIS"]

    CppApplication {
        cpp.includePaths: ["."]
        cpp.commonCompilerFlags: ["-Wno-unused-parameter"]
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
            "usb/usb_io.c",
            "usb/usb_io.h",
            "usb/usb_irq.c",
            "usb/usb_macros.h",
        ]

        FileTagger {
            patterns: "*.ld"
            fileTags: ["linkerscript"]
        }
    }
}
