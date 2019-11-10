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
    cpp.includePaths: ["common", "."]

    Properties {
        condition: qbs.buildVariant == "debug"
        cpp.defines: outer.concat(["DEBUG=5"])
        cpp.optimization: "none"
    }

    Properties {
        condition: qbs.buildVariant == "release"
        cpp.defines: outer.concat(["DEBUG=1"])
        cpp.optimization: "fast"
    }

    Properties {
        condition: project.bootloader
        cpp.defines: outer.concat(["BOOTLOADER"])
    }

    Export {
        Depends {name: "cpp"}
        Depends {name: "CMSIS"}
        Parameters {cpp.linkWholeArchive: true}
        cpp.includePaths: product.cpp.includePaths
        cpp.defines: product.cpp.defines
        cpp.driverFlags: product.cpp.driverFlags
        cpp.optimization: product.cpp.optimization
    }

    files: [
        "common/escape.h",
        "common/list.h",
        "common/macros.h",
        "common/debug.h",
        "common/device.h",
        "system/syscall.c",
        "system/system.c",
        "system/system.h",
        "system/system.s",
        "system/clocks.h",
        "system/clocks.c",
        "system/systick.h",
        "system/systick.c",
        "system/irq.h",
        "system/dma.txt",
        "peripheral/led.h",
        "peripheral/led.c",
        "peripheral/keyboard.h",
        "peripheral/keyboard.c",
        "usb/ep0/usb_ep0.c",
        "usb/ep0/usb_ep0.h",
        "usb/usb.h",
        "usb/usb_core.c",
        "usb/usb_core.h",
        "usb/usb_macros.h",
        "usb/usb_hw.c",
        "usb/usb_hw.h",
        "usb/usb_hw_macros.h",
    ]
}
