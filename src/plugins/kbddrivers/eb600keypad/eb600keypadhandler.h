/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef EB600KEYPADHANDLER_H
#define EB600KEYPADHANDLER_H

#include <QObject>
#include <QWSKeyboardHandler>

class QSocketNotifier;
class EB600KeypadHandler : public QObject, public QWSKeyboardHandler {
    Q_OBJECT
public:
    EB600KeypadHandler(const QString &device = QString("/dev/input/event0"));
    ~EB600KeypadHandler();

private:

#if 0 //2440
    enum keys {
        EB600_KEY_PWR = 0x0d ,
        EB600_KEY_HOME = 0x04,
        EB600_KEY_HOTKEY = 0x03,
        EB600_KEY_ROTATE = 0x02,
        EB600_KEY_MENU = 0x05,
        EB600_KEY_UP = 0x07,
        EB600_KEY_DOWN = 0x08,
        EB600_KEY_RIGHT = 0x0a,
        EB600_KEY_LEFT = 0x09,
        EB600_KEY_ENTER = 0x06 ,
        EB600_KEY_VOLUME_UP = 0x0b ,
        EB600_KEY_VOLUME_DOWN = 0x0c
    };
#else
    enum keys {
        EB600_KEY_PWR = 0x74 ,
        EB600_KEY_HOME = 0x3d,	//F3
        EB600_KEY_HOTKEY = 0x03,
        EB600_KEY_BACK = 0x1c,
        EB600_KEY_ROTATE = 0x02,
        EB600_KEY_MENU = 0x3e,	//F4

        EB600_KEY_UP = 0x67,
        EB600_KEY_DOWN = 0x6c,
        EB600_KEY_RIGHT = 0x6a,
        EB600_KEY_LEFT = 0x69,
        EB600_KEY_ENTER = 0x10,
        EB600_KEY_VOLUME_UP = 0x41, //F7
        EB600_KEY_VOLUME_DOWN = 0x42 //F8
    };
#endif
    QSocketNotifier *m_notify;
    int  kbdFD;
    bool shift;

private Q_SLOTS:
    void readKbdData();
};

#endif // EB600KEYPADHANDLER_H
