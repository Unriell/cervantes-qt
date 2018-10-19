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

#include "eb600keypaddriverplugin.h"
#include "eb600keypadhandler.h"

#include <qdebug.h>
#include <stdio.h>

#if 1
#define qLog(x) qDebug()
#else
#define qLog(x) while (0) qDebug()
#endif

EB600KeypadDriverPlugin::EB600KeypadDriverPlugin( QObject *parent )
    : QKbdDriverPlugin( parent )
{
    qDebug() << "[DBG] " << __FILE__ << ", " << __FUNCTION__ << ", " << __LINE__;
}

EB600KeypadDriverPlugin::~EB600KeypadDriverPlugin()
{
    qDebug() << "[DBG] " << __FILE__ << ", " << __FUNCTION__ << ", " << __LINE__;
}

//QWSKeyboardHandler* EB600KeypadDriverPlugin::create(const QString &driver, const QString &device)
EB600KeypadHandler* EB600KeypadDriverPlugin::create(const QString &driver, const QString &device)
{
//     if (device.isEmpty()) {
// 	return create( driver );
//     }
    qDebug() << "[DBG] " << __FILE__ << ", " << __FUNCTION__ << ", " << __LINE__;
    if( driver.toLower() == "eb600keypad") {
        qLog(Input) << "Before call EB600keypadHandler(" << device << ")";
        return new EB600KeypadHandler(device);
    }
    return 0;
}

//QWSKeyboardHandler* EB600KeypadDriverPlugin::create( const QString &driver)
EB600KeypadHandler* EB600KeypadDriverPlugin::create( const QString &driver)
{
    qDebug() << "[DBG] " << __FILE__ << ", " << __FUNCTION__ << ", " << __LINE__;
    if( driver.toLower() == "eb600keypad") {
        qLog(Input) << "Before call EB600keypadHandler()";
        return new EB600KeypadHandler();
    }
    return 0;
}

QStringList EB600KeypadDriverPlugin::keys() const
{
    qDebug() << "[DBG] " << __FILE__ << ", " << __FUNCTION__ << ", " << __LINE__;
    return QStringList() << "eb600keypad";
}

//Q_EXPORT_PLUGIN2(qwslinuxiskbdhandler, EB600KeypadDriverPlugin)
Q_EXPORT_PLUGIN2(eb600keypad, EB600KeypadDriverPlugin)
