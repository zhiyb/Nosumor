import qbs

Project {
    property string device: "STM32F722"
    property bool cmsis_dsp: false

    references: ["CMSIS"]

    CppApplication {
        cpp.commonCompilerFlags: [
            "-Wno-unused-parameter",
            "-Wno-unused-function",
        ]
        cpp.staticLibraries: ["m"]
        cpp.includePaths: ["."]
        Depends {name: "CMSIS"}

        Properties {
            condition: qbs.buildVariant == "debug"
            cpp.defines: ["DEBUG"]
        }

        Properties {
            condition: qbs.buildVariant == "release"
            cpp.optimization: "small"
        }

        Group {
            name: "FatFS"
            cpp.optimization: "small"
            cpp.commonCompilerFlags: outer.concat(["-Wno-comment"])
            files: [
                "fatfs/*",
            ]
        }

        Group {
            name: "Peripherals"
            files: [
                "peripheral/audio.c",
                "peripheral/audio.h",
                "peripheral/mmc_defs.h",
                "peripheral/mmc.c",
                "peripheral/mmc.h",
                "peripheral/uart.c",
                "peripheral/uart.h",
                "peripheral/keyboard.c",
                "peripheral/keyboard.h",
            ]
        }

        Group {
            name: "USB interfaces"
            files: [
                "usb/audio/usb_audio.c",
                "usb/audio/usb_audio.h",
                "usb/audio/usb_audio_defs.h",
                "usb/audio/usb_audio_desc.h",
            ]
        }

        Group {
            name: "USB HID interface"
            files: [
                "usb/hid/usb_hid.c",
                "usb/hid/usb_hid.h",
                "usb/hid/keyboard/usb_hid_keyboard.c",
                "usb/hid/keyboard/usb_hid_keyboard.h",
            ]
        }

        Group {
            name: "USB"
            files: [
                "usb/usb.c",
                "usb/usb.h",
                "usb/usb_debug.h",
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
        }

        Group {
            name: "System"
            files: [
                "STM32F722RETx_FLASH.ld",
                "clocks.c",
                "clocks.h",
                "pvd.c",
                "pvd.h",
                "startup_stm32f722xx.S",
                "syscalls.c",
                "system_stm32f7xx.c",
                "debug.h",
                "systick.c",
                "systick.h",
                "fio.c",
                "fio.h",
            ]
        }

        files: [
            "irq.h",
            "escape.h",
            "macros.h",
            "main.c",
        ]

        FileTagger {
            patterns: "*.ld"
            fileTags: ["linkerscript"]
        }

        Group {     // Properties for the produced executable
            fileTagsFilter: product.type
            qbs.install: true
        }
    }
}
