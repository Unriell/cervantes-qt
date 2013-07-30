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

#include "eb600keypadhandler.h"

#include <QSocketNotifier>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <linux/kd.h>
#include <linux/input.h>

#include <qdebug.h>
#if 1
#define qLog(x) qDebug()
#else
#define qLog(x) while (0) qDebug()
#endif

EB600KeypadHandler::EB600KeypadHandler(const QString &device)
{
    qDebug() << "[DBG] " << __FILE__ << ", " << __FUNCTION__ << ", " << __LINE__;
    qLog(Input) << "Loaded EB600 keypad plugin!";
    setObjectName( "EB600Keypad Handler" );
    // ES modify for correct kbd reading

    //  kbdFD = ::open(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY);
    kbdFD = ::open(device.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);

    if (kbdFD >= 0) {
        qLog(Input) << "Opened" << device << "as keypad input";
        m_notify = new QSocketNotifier( kbdFD, QSocketNotifier::Read, this );
        connect( m_notify, SIGNAL(activated(int)), this, SLOT(readKbdData()));
    } else {
        qWarning("Cannot open '%s' for keypad (%s)",
                 device.toLocal8Bit().constData(), strerror(errno));
        return;
    }
    shift = false;
}

EB600KeypadHandler::~EB600KeypadHandler()
{
    if(kbdFD >= 0)
        ::close(kbdFD);
}


void EB600KeypadHandler::readKbdData()
{
    printf("[DBG] %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
#if 0
    uint8_t buf[32];

    uint n = ::read(kbdFD, buf, sizeof(buf));

    if (n == 0 || n == 0xFFFFFFFF)
        return;
//     qLog() << "read finished, n = " << n << ", sizeof(iebuf) = " << sizeof(buf) << ", buf[0] = " << buf[0];
    bool pressed;
    bool autorepeat;
    int modifiers = 0;
    int unicode = 0xffff;
    int keycode;

    n /= sizeof(uint8_t);

  //   qLog() << "before loop, n = " << n << ", sizeof(uint8_t) = " << sizeof(uint8_t);
    for (uint i = 0; i < n; i++) {
        pressed = true;
        autorepeat = false;

  //      qLog() << "keyEvent" << hex << buf[i];

        switch (buf[i]) {
            case EB600_KEY_PWR:
                keycode = Qt::Key_Context1;
                break;
            case EB600_KEY_ROTATE:
                keycode = Qt::Key_F22;//Key_Home;
                break;
            case EB600_KEY_HOTKEY:
                keycode = Qt::Key_Tab;
                break;
            case EB600_KEY_HOME:
                keycode = Qt::Key_F24;//Key_Escape;
                break;
            case EB600_KEY_MENU:
                keycode = Qt::Key_F25;//Key_Menu;
                break;
            case EB600_KEY_UP:
                keycode = Qt::Key_Up;
                break;
            case EB600_KEY_DOWN:
                keycode = Qt::Key_Down;
                break;
            case EB600_KEY_RIGHT:
                keycode = Qt::Key_Right;
                break;
            case EB600_KEY_LEFT://ie->value != 0;
                keycode = Qt::Key_Left;
                break;
            case EB600_KEY_ENTER:
                keycode = Qt::Key_Enter;
                break;
            case EB600_KEY_VOLUME_UP:
                keycode = Qt::Key_F26;//Key_VolumeUp;
                break;
            case EB600_KEY_VOLUME_DOWN:
                keycode = Qt::Key_F27;//Key_VolumeDown;
                break;
            default:
                keycode = Qt::Key_unknown;
                break;
        }

        if (keycode != Qt::Key_unknown)
        {
     //       qLog() << "pressed" << hex << buf[12];
            // ES: for 357 keyrelease
            if (buf[12] == 0){
                  //  keycode |= 0x80;
                    pressed = false;
                }
            else{
                keycode = keycode;
                pressed = true;
            }
        //     qLog() << "process key event: " << keycode << ", buf["<<i<<"] = " << hex << buf[i];
            processKeyEvent(unicode, keycode, (Qt::KeyboardModifiers)modifiers,
                            pressed, autorepeat);
        }
    }
//     qLog() << "exist loop, and return readKbdData";
#else
    int i, tot;
    struct input_event ev[16];

    bool autorepeat;
    int modifiers = 0;
    int unicode = 0xffff;
    int keycode;
    int ntx_io = -1;

    tot = read( kbdFD, &ev, sizeof(ev));

    if( (tot < sizeof(struct input_event)) || (tot <= 0) ) {

            if (EAGAIN != errno) {
               printf ("[%s-%d] tot %d, unexpected errno %d !!!!!!!!\n", __func__,__LINE__, tot, errno);
               if (EBADF == errno) {
                    printf ("[%s-%d] EBADF %d\n", __func__,__LINE__, kbdFD);
                    kbdFD = open( "/dev/input/event0", O_RDONLY | O_NONBLOCK ); // kilton 20091204.
                    }
                return;
                }
        }

            for (i=0; i < tot / sizeof(struct input_event); i++) {
                        if( ev[i].type == EV_SYN )
                        {
                            printf("Event: time %ld.%06ld, -------------- %s ------------\n",
                            ev[i].time.tv_sec, ev[i].time.tv_usec,
                            ev[i].code ? "Config Sync" : "Report Sync" );
                        }
                        else
                        {
                            printf( "ev[i].code : 0x%02X\n", ev[i].code);
                            printf( "ev[i].type : 0x%02X\n", ev[i].type);
                            printf( "ev[i].value : 0x%02X\n", ev[i].value);

                            switch( ev[i].code )
                            {
                            case EB600_KEY_PWR:
                                printf("EB600_KEY_PWR\n");
                                keycode = Qt::Key_F20;
                                break;
                            case EB600_KEY_ROTATE:
                                printf("EB600_KEY_ROTATE\n");
                                keycode = Qt::Key_F21;//Key_Home;
                                break;
                            case EB600_KEY_HOTKEY:
                                printf("EB600_KEY_HOTKEY\n");
                                keycode = Qt::Key_F22;
                                break;
                            case EB600_KEY_HOME:
                                printf("EB600_KEY_HOME\n");
                                keycode = Qt::Key_F23;//Key_Escape;
                                break;
                            case EB600_KEY_BACK:
                                printf("EB600_KEY_BACK\n");
                                keycode = Qt::Key_F24;
                                break;
                            case EB600_KEY_MENU:
                                 printf("EB600_KEY_MENU\n");
                                keycode = Qt::Key_F25;//Key_Menu;
                                break;
                            case EB600_KEY_UP:
                                printf("EB600_KEY_UP\n");
                                keycode = Qt::Key_Up;
                                break;
                            case EB600_KEY_DOWN:
                                printf("EB600_KEY_DOWN\n");
                                keycode = Qt::Key_Down;
                                break;
                            case EB600_KEY_RIGHT:
                                printf("EB600_KEY_RIGHT\n");
                                keycode = Qt::Key_Right;
                                break;
                            case EB600_KEY_LEFT://ie->value != 0;
                                printf("EB600_KEY_LEFT\n");
                                keycode = Qt::Key_Left;
                                break;
                            case EB600_KEY_ENTER:
                                keycode = Qt::Key_Enter;
                                printf("EB600_KEY_ENTER\n");
                                break;
                            case EB600_KEY_VOLUME_UP:
                                printf("EB600_KEY_VOLUME_UP\n");
                                keycode = Qt::Key_VolumeUp;//Key_VolumeUp;
                                break;
                            case EB600_KEY_VOLUME_DOWN:
                                 printf("EB600_KEY_VOLUME_DOWN\n");
                                keycode = Qt::Key_VolumeDown;//Key_VolumeDown;
                                break;
                            default:
                                keycode = Qt::Key_unknown;
                                keycode = 0;
                                printf("[DBG] Unknow keycode\n");
                                break;
                            }

                            if (keycode != 0)
                            {
                                switch( ev[i].value )
                                {
                                        case 1: // press
                                  //          printf ("[%s-%d] %d pressed.\n",__func__,__LINE__,keycode);
                                            while ((i+1) < (tot / sizeof(struct input_event))) {
                                              if (0 != ev[i+1].value)
                                                ++i;
                                            else
                                                break;
                                            }
                                            if ((i+1) < (tot / sizeof(struct input_event)))
                                            continue;
                                            processKeyEvent(unicode, keycode, (Qt::KeyboardModifiers)modifiers,
                                                            true, autorepeat);

                                            // led on
                                            ntx_io = open("/dev/ntx_io", O_RDWR);
                                            ioctl(ntx_io, 101, 0);
                                            close(ntx_io);


                                            break;

                                    case 0: // release
                                         processKeyEvent(unicode, keycode, (Qt::KeyboardModifiers)modifiers,
                                                        false, autorepeat);

                                         // led off
                                         ntx_io = open("/dev/ntx_io", O_RDWR);
                                         ioctl(ntx_io, 127, 0);
                                         close(ntx_io);

                                        break;
                                    case 2: // repeat char
                                        keycode= 0;
                                        break;
                                    }
                            }
                        }
        }

#endif
}

