import qbs

Project {
    minimumQbsVersion: "1.7.1"

    QtGuiApplication {
        Depends {name: "Qt.widgets"}
        Depends {name: "Nosumor"}

        files: [
            "devicewidget.cpp",
            "devicewidget.h",
            "main.cpp",
            "mainwindow.cpp",
            "mainwindow.h",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: product.type
            qbs.install: true
        }
    }
}
