import qbs

StaticLibrary {
    name: "core"
    Depends {name: "CMSIS"}

    cpp.defines: {
        var flags = ["HWVER=" + project.hwver];
        if (project.bootloader)
            flags.push("BOOTLOADER");
        return flags;
    }

    cpp.driverFlags: {
        var flags = [];
        if (project.bootloader)
            flags.push("--specs=nano.specs");
        return flags;
    }

    cpp.commonCompilerFlags: [
        "-Wno-unused-parameter",
        "-Wno-unused-function",
        "-Wno-unused-variable",
    ]
    cpp.staticLibraries: ["m"]
    cpp.includePaths: [".", "api", "eMPL"]

    Properties {
        condition: qbs.buildVariant == "debug"
        cpp.defines: outer.concat(["DEBUG"])
    }

    Properties {
        condition: qbs.buildVariant == "release"
        cpp.optimization: "fast"
    }

    Properties {
        condition: project.bootloader
        cpp.defines: outer.concat(["BOOTLOADER"])
    }

    Export {
        Depends {name: "cpp"}
        Depends {name: "CMSIS"}
        cpp.includePaths: [".", "api"]
        cpp.defines: product.cpp.defines
        cpp.driverFlags: product.cpp.driverFlags
        cpp.optimization: product.cpp.optimization
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
            "peripheral/mpu.c",
            "peripheral/mpu.h",
            "peripheral/mpu_defs.h",
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
        name: "USB HID"
        files: [
            "usb/hid/usb_hid.c",
            "usb/hid/usb_hid.h",
            "usb/hid/vendor/usb_hid_vendor.c",
            "usb/hid/vendor/usb_hid_vendor.h",
            "usb/hid/keyboard/usb_hid_keyboard.c",
            "usb/hid/keyboard/usb_hid_keyboard.h",
            "usb/hid/mouse/usb_hid_mouse.c",
            "usb/hid/mouse/usb_hid_mouse.h",
            "usb/hid/joystick/usb_hid_joystick.c",
            "usb/hid/joystick/usb_hid_joystick.h",
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
        name: "System Modules"
        cpp.commonCompilerFlags: outer.concat(["-Wno-array-bounds"])
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
        name: "Vendor HID API"
        files: [
            "api/api.h",
            "api/api_defs.h",
            "api/api_keycode.c",
            "api/api_keycode.h",
            "api/api_ping.c",
            "api/api_ping.h",
            "api/api_proc.c",
            "api/api_proc.h",
            "api/api_led.c",
            "api/api_led.h",
            "api/api_i2c.c",
            "api/api_i2c.h",
            "api/api_config.c",
            "api/api_config.h",
            "api/api_config_priv.h",
            "api/api_motion.c",
            "api/api_motion.h",
        ]
    }

    Group {
        name: "Logic Modules"
        files: [
            "logic/led_trigger.c",
            "logic/led_trigger.h",
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

    Group {
        name: "FatFS"
        cpp.commonCompilerFlags: outer.concat(["-Wno-comment",
                                               "-Wno-implicit-fallthrough"])
        cpp.optimization: "fast"
        files: [
            "fatfs/*",
        ]
    }

    Group {
        name: "Motion Driver"
        cpp.commonCompilerFlags: outer.concat(["-Wno-maybe-uninitialized"])
        cpp.defines: outer.concat(["MPU9250", "EMPL_TARGET_STM32F4"])
        files: [
            "eMPL/*",
        ]
    }

    files: [
        "irq.h",
        "escape.h",
        "macros.h",
        "debug.h",
    ]
}
