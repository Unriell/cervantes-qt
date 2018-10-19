SOURCES += main.cpp
RESOURCES += states.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/animation/states
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS states.pro \
                accessories-dictionary.png akregator.png digikam.png \
                help-browser.png k3b.png kchart.png
sources.path = $$[QT_INSTALL_EXAMPLES]/animation/states
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000E3F8
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)
