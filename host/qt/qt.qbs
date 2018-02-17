import qbs

QtGuiApplication {
    consoleApplication: false
    Depends {name: "Qt.widgets"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}
    Depends {name: "plugin.info"}

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
