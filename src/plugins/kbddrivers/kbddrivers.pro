TEMPLATE = subdirs
contains(kbd-plugins, linuxinput): SUBDIRS += linuxinput
contains(kbd-plugins, eb600keypad): SUBDIRS += eb600keypad
