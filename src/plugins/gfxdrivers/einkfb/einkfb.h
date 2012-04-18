/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QSCREENEINKFB_QWS_H
#define QSCREENEINKFB_QWS_H

#include <QtGui/qscreen_qws.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_QWS_EINKFB

class EInkFb_Shared
{
public:
    volatile int lastop;
    volatile int optype;
    volatile int fifocount;   // Accel drivers only
    volatile int fifomax;
    volatile int forecol;     // Foreground colour caching
    volatile unsigned int buffer_offset;   // Destination
    volatile int linestep;
    volatile int cliptop;    // Clip rectangle
    volatile int clipleft;
    volatile int clipright;
    volatile int clipbottom;
    volatile unsigned int rop;

};

struct fb_cmap;
struct fb_var_screeninfo;
struct fb_fix_screeninfo;
class EInkFbScreenPrivate;

enum EInkFbModes {
    MODE_EINK_AUTO,
    MODE_EINK_BLOCK,
    MODE_EINK_SAFE,
    MODE_EINK_QUICK,
    MODE_EINK_FASTEST
};

enum EInkFbUpdateSchemes {
    SCHEME_EINK_QUEUE,
    SCHEME_EINK_MERGE
};

class Q_GUI_EXPORT EInkFbScreen : public QScreen
{
public:
    explicit EInkFbScreen(int display_id);
    virtual ~EInkFbScreen();

    virtual void exposeRegion(QRegion region, int changing);
    virtual void setDirty(const QRect&);

    virtual bool initDevice();
    virtual bool connect(const QString &displaySpec);

    virtual bool useOffscreen();

    virtual void disconnect();
    virtual void shutdownDevice();
    virtual void setMode(int,int,int);
    virtual void save();
    virtual void restore();
    virtual void blank(bool on);
    virtual void set(unsigned int,unsigned int,unsigned int,unsigned int);
    virtual uchar * cache(int);
    virtual void uncache(uchar *);
    virtual int sharedRamSize(void *);

    EInkFb_Shared * shared;

    virtual void setWaveForm(int);
    
    virtual void setRefreshMode(int mode, bool justOnce);
    virtual void setUpdateScheme(int newScheme, bool justOnce);
    virtual void blockUpdates();
    virtual void unblockUpdates();

protected:

    void deleteEntry(uchar *);

    bool canaccel;
    int dataoffset;
    int cacheStart;

    static void clearCache(QScreen *instance, int);
    int m_waveform;

    int haltUpdates;
    int haltCount;
    int currentMode;
    int currentFlags;
    int currentScheme;
    int useModeOnce;
    int useSchemeOnce;
    int fullUpdates;

    int previousMode;
    int previousFlags;
    int previousScheme;

private:

    void delete_entry(int);
    void insert_entry(int,int,int);
    void setupOffScreen();
    void createPalette(fb_cmap &cmap, fb_var_screeninfo &vinfo, fb_fix_screeninfo &finfo);
    void setPixelFormat(struct fb_var_screeninfo);

	void setupController();
	void restoreController();
	unsigned reg140;
	unsigned reg32c;
	unsigned reg330;
	
    EInkFbScreenPrivate *d_ptr;
};

#endif // QT_NO_QWS_EINKFB

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSCREENEINKFB_QWS_H
