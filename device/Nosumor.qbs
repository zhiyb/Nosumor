import qbs

Project {
    name: "Nosumor (device)"
    minimumQbsVersion: "1.7.1"

    property string device: "STM32F722"
    property bool cmsis_dsp: false
    property int hwver: 0x0003

    references: ["CMSIS"]

    CppApplication {
        name: "Nosumor"
        type: ["application", "hex", "size"]
        Depends {name: "core"}
        Depends {name: "gcc-none"}

        files: [
            "main.c",
            "logic/scsi_buf.c",
            "logic/vendor.c",
        ]

        Group {
            name: "FatFS"
            cpp.commonCompilerFlags: outer.concat(["-Wno-comment"])
            files: [
                "fatfs/*",
            ]
        }

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

    CppApplication {
        name: "Bootloader"
        type: ["application", "hex", "size"]
        Depends {name: "core"}
        Depends {name: "gcc-none"}

        cpp.defines: ["BOOTLOADER"]
        cpp.driverFlags: ["--specs=nano.specs"]

        files: [
            "bootloader.c",
            "logic/scsi_buf.c",
            "logic/vendor.c",
        ]

        Group {
            name: "FatFS"
            cpp.commonCompilerFlags: outer.concat(["-Wno-comment"])
            files: [
                "fatfs/*",
            ]
        }

        Group {
            name: "Linker script for AXI"
            condition: qbs.buildVariant === "debug"
            files: "STM32F722RETx_FLASH_AXIM_BL.ld"
        }

        Group {
            name: "Linker script for ITCM BL"
            condition: qbs.buildVariant === "release"
            files: "STM32F722RETx_FLASH_ITCM_BL.ld"
        }

        Group {     // Properties for the produced executable
            fileTagsFilter: ["application", "hex", "bin"]
            qbs.install: true
        }
    }

    StaticLibrary {
        name: "core"
        Depends {name: "CMSIS"}

        cpp.defines: ["HWVER=" + hwver]

        cpp.commonCompilerFlags: [
            "-Wno-unused-parameter",
            "-Wno-unused-function",
            "-Wno-unused-variable",
        ]
        cpp.staticLibraries: ["m"]
        cpp.includePaths: ["."]

        Properties {
            condition: qbs.buildVariant == "debug"
            cpp.defines: outer.concat(["DEBUG"])
        }

        Properties {
            condition: qbs.buildVariant == "release"
            cpp.optimization: "small"
        }

        Export {
            Depends {name: "cpp"}
            Depends {name: "CMSIS"}
            cpp.includePaths: ["."]
            cpp.defines: product.cpp.defines
        }

        Group {
            name: "Peripherals"
            files: [
                "peripheral/audio.c",
                "peripheral/audio.h",
                "peripheral/audio_config.c",
                "peripheral/i2c.c",
                "peripheral/i2c.h",
                "peripheral/led.c",
                "peripheral/led.h",
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
            name: "USB Audio Class 1"
            // Not working yet
            condition: false
            files: [
                "usb/audio/usb_audio.c",
                "usb/audio/usb_audio.h",
                "usb/audio/usb_audio_defs.h",
                "usb/audio/usb_audio_desc.h",
            ]
        }

        Group {
            name: "USB Audio Class 2"
            files: [
                "usb/audio2/usb_audio2.c",
                "usb/audio2/usb_audio2.h",
                "usb/audio2/usb_audio2_cs.c",
                "usb/audio2/usb_audio2_cx.c",
                "usb/audio2/usb_audio2_defs.h",
                "usb/audio2/usb_audio2_defs_frmts.h",
                "usb/audio2/usb_audio2_defs_spatial.h",
                "usb/audio2/usb_audio2_defs_termt.h",
                "usb/audio2/usb_audio2_desc.h",
                "usb/audio2/usb_audio2_entities.c",
                "usb/audio2/usb_audio2_entities.h",
                "usb/audio2/usb_audio2_ep_data.c",
                "usb/audio2/usb_audio2_ep_data.h",
                "usb/audio2/usb_audio2_ep_feedback.c",
                "usb/audio2/usb_audio2_ep_feedback.h",
                "usb/audio2/usb_audio2_fu.c",
                "usb/audio2/usb_audio2_structs.h",
                "usb/audio2/usb_audio2_term.c",
            ]
        }

        Group {
            name: "USB HID interface"
            files: [
                "usb/hid/usb_hid.c",
                "usb/hid/usb_hid.h",
                "usb/hid/keyboard/usb_hid_keyboard.c",
                "usb/hid/keyboard/usb_hid_keyboard.h",
                "usb/hid/vendor/usb_hid_vendor.c",
                "usb/hid/vendor/usb_hid_vendor.h",
            ]
        }

        Group {
            name: "USB Mass Storage"
            files: [
                "usb/msc/usb_msc.c",
                "usb/msc/usb_msc.h",
                "usb/msc/usb_msc_defs.h",
            ]
        }

        Group {
            name: "USB"
            files: [
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
                "usb/usb_structs.h",
            ]
        }

        Group {
            name: "System modules"
            files: [
                "startup_stm32f722xx.S",
                "system/dma.txt",
                "system_stm32f7xx.c",
                "system/clocks.c",
                "system/clocks.h",
                "system/pvd.c",
                "system/pvd.h",
                "system/systick.c",
                "system/systick.h",
                "system/flash.c",
                "system/flash.h",
                "system/flash_scsi.c",
                "system/flash_scsi.h",
            ]
        }

        Group {
            name: "Logic modules"
            files: [
                "vendor_defs.h",
                "logic/vendor.h",
                "logic/syscalls.c",
                "logic/fio.c",
                "logic/fio.h",
                "logic/scsi.c",
                "logic/scsi.h",
                "logic/scsi_defs.h",
                "logic/scsi_defs_cmds.h",
                "logic/scsi_defs_ops.h",
                "logic/scsi_defs_sense.h",
            ]
        }

        files: [
            "irq.h",
            "escape.h",
            "macros.h",
            "debug.h",
        ]
    }
}
