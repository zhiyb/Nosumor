import qbs

Project {
    property string device: "STM32F722"
    property bool cmsis_dsp: false

    minimumQbsVersion: "1.7.1"
    references: ["CMSIS"]

    CppApplication {
        Depends {name: "CMSIS"}

        Properties {
            condition: qbs.buildVariant == "debug"
            cpp.defines: ["DEBUG"]
        }

        Properties {
            condition: qbs.buildVariant == "release"
            cpp.optimization: "small"
        }

        files: ["*.S", "*.c", "*.h", "*.ld"]

        FileTagger {
            patterns: "*.ld"
            fileTags: ["linkerscript"]
        }
    }
}
