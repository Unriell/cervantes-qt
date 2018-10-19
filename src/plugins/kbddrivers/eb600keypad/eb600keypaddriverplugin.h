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

#ifndef EB600KEYPADDRIVERPLUGIN_H
#define EB600KEYPADDRIVERPLUGIN_H

#include <QtGui/QWSKeyboardHandlerFactoryInterface>
#include "eb600keypadhandler.h"

class EB600KeypadDriverPlugin : public QKbdDriverPlugin {
    Q_OBJECT
public:
    EB600KeypadDriverPlugin( QObject *parent  = 0 );
    ~EB600KeypadDriverPlugin();

//    QWSKeyboardHandler* create(const QString& driver, const QString& device);
    EB600KeypadHandler* create(const QString& driver, const QString& device);
//    QWSKeyboardHandler* create(const QString& driver);
    EB600KeypadHandler* create(const QString& driver);
    QStringList keys()const;
};

#endif // EB600KEYPADDRIVERPLUGIN_H
