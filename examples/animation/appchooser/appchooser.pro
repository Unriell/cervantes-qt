SOURCES = main.cpp
RESOURCES = appchooser.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/animation/appchooser
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS appchooser.pro \
                accessories-dictionary.png akregator.png digikam.png k3b.png
sources.path = $$[QT_INSTALL_EXAMPLES]/animation/appchooser
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000E3F5
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)
