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

#include "einkfb.h"

#include <QDebug>

#ifndef QT_NO_QWS_EINKFB
//#include "qmemorymanager_qws.h"
#include "qwsdisplay_qws.h"
#include "qpixmap.h"
#include <private/qwssignalhandler_p.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <linux/mxcfb.h>

#include "qwindowsystem_qws.h"
#include <linux/fb.h>

QT_BEGIN_NAMESPACE

extern int qws_client_id;

#define WAVEFORM_MODE_INIT	0x0	/* init mode, turn the screen white */
#define WAVEFORM_MODE_DU	0x1	/* fast 1bit update without flashing */
#define WAVEFORM_MODE_GC16	0x2	/* High fidelity (flashing) */
/* 0x3 is empty */
#define WAVEFORM_MODE_A2	0x4	/* Fast page flipping at reduced contrast */
#define WAVEFORM_MODE_GL16	0x5	/* not everywhere */

//#define DEBUG_CACHE

class EInkFbScreenPrivate : public QObject
{
public:
    EInkFbScreenPrivate();
    ~EInkFbScreenPrivate();

    void openTty();
    void closeTty();

    int fd;
    int startupw;
    int startuph;
    int startupd;
    bool blank;

    bool doGraphicsMode;
#ifdef QT_QWS_DEPTH_GENERIC
    bool doGenericColors;
#endif
    int ttyfd;
    long oldKdMode;
    QString ttyDevice;
    QString displaySpec;
};

EInkFbScreenPrivate::EInkFbScreenPrivate()
    : fd(-1), blank(true), doGraphicsMode(true),
#ifdef QT_QWS_DEPTH_GENERIC
      doGenericColors(false),
#endif
      ttyfd(-1), oldKdMode(KD_TEXT)
{
    QWSSignalHandler::instance()->addObject(this);
}

EInkFbScreenPrivate::~EInkFbScreenPrivate()
{
    closeTty();
}

void EInkFbScreenPrivate::openTty()
{
    const char *const devs[] = {"/dev/tty0", "/dev/tty", "/dev/console", 0};

    if (ttyDevice.isEmpty()) {
        for (const char * const *dev = devs; *dev; ++dev) {
            ttyfd = ::open(*dev, O_RDWR);
            if (ttyfd != -1)
                break;
        }
    } else {
        ttyfd = ::open(ttyDevice.toAscii().constData(), O_RDWR);
    }

    if (ttyfd == -1)
        return;

    if (doGraphicsMode) {
        ioctl(ttyfd, KDGETMODE, &oldKdMode);
        if (oldKdMode != KD_GRAPHICS) {
            int ret = ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
            if (ret == -1)
                doGraphicsMode = false;
        }
    }

    // No blankin' screen, no blinkin' cursor!, no cursor!
    const char termctl[] = "\033[9;0]\033[?33l\033[?25l\033[?1c";
    ::write(ttyfd, termctl, sizeof(termctl));
}

void EInkFbScreenPrivate::closeTty()
{
    if (ttyfd == -1)
        return;

    if (doGraphicsMode)
        ioctl(ttyfd, KDSETMODE, oldKdMode);

    // Blankin' screen, blinkin' cursor!
    const char termctl[] = "\033[9;15]\033[?33h\033[?25h\033[?0c";
    ::write(ttyfd, termctl, sizeof(termctl));

    ::close(ttyfd);
    ttyfd = -1;
}

/*!
    \internal

    \class QLinuxFbScreen
    \ingroup qws

    \brief The QLinuxFbScreen class implements a screen driver for the
    Linux framebuffer.

    Note that this class is only available in \l{Qt for Embedded Linux}.
    Custom screen drivers can be added by subclassing the
    QScreenDriverPlugin class, using the QScreenDriverFactory class to
    dynamically load the driver into the application, but there should
    only be one screen object per application.

    The QLinuxFbScreen class provides the cache() function allocating
    off-screen graphics memory, and the complementary uncache()
    function releasing the allocated memory. The latter function will
    first sync the graphics card to ensure the memory isn't still
    being used by a command in the graphics card FIFO queue. The
    deleteEntry() function deletes the given memory block without such
    synchronization.  Given the screen instance and client id, the
    memory can also be released using the clearCache() function, but
    this should only be necessary if a client exits abnormally.

    In addition, when in paletted graphics modes, the set() function
    provides the possibility of setting a specified color index to a
    given RGB value.

    The QLinuxFbScreen class also acts as a factory for the
    unaccelerated screen cursor and the unaccelerated raster-based
    implementation of QPaintEngine (\c QRasterPaintEngine);
    accelerated drivers for Linux should derive from this class.

    \sa QScreen, QScreenDriverPlugin, {Running Applications}
*/

/*!
    \fn bool QLinuxFbScreen::useOffscreen()
    \internal
*/

// Unaccelerated screen/driver setup. Can be overridden by accelerated
// drivers

/*!
    \fn QLinuxFbScreen::QLinuxFbScreen(int displayId)

    Constructs a QLinuxFbScreen object. The \a displayId argument
    identifies the Qt for Embedded Linux server to connect to.
*/

EInkFbScreen::EInkFbScreen(int display_id)
    : QScreen(display_id, LinuxFBClass), marker_val(1), d_ptr(new EInkFbScreenPrivate)
{
    canaccel=false;
    clearCacheFunc = &clearCache;
#ifdef QT_QWS_CLIENTBLIT
    setSupportsBlitInClients(true);
#endif
    
    useModeOnce = 0;
    useSchemeOnce = 0;
    currentMode = WAVEFORM_MODE_GC16;
    currentFlags = 0;
    haltUpdates = 0;
    haltCount = 0;
    queueCount = 0;
    pendingPresent = false;
}

/*!
    Destroys this QLinuxFbScreen object.
*/

EInkFbScreen::~EInkFbScreen()
{
}

void EInkFbScreen::setRefreshMode(int mode, bool justOnce)
{
    setRefreshMode(mode, 0, justOnce);
}

void EInkFbScreen::setRefreshMode(int mode, int newFlags, bool justOnce)
{
    int newMode = -1;

    qDebug() << "setRefreshMode" << __func__ << mode << newFlags << justOnce;

    switch(mode) {
      case MODE_EINK_BLOCK:
	  /* we only block further updates
	   * so there is nothing more to do
	   */
	  newMode = currentMode;
	  break;
      case MODE_EINK_SAFE:
	  newMode = WAVEFORM_MODE_GC16;
	  newFlags |= FLAG_REFRESH;
	  break;
      case MODE_EINK_QUICK:
	  newMode = WAVEFORM_MODE_GC16;
	  break;
      case MODE_EINK_FAST:
	  newMode = WAVEFORM_MODE_DU;
	  break;
      case MODE_EINK_FASTEST:
	  newMode = WAVEFORM_MODE_A2;
	  break;
      case MODE_EINK_AUTO:
	  newMode = WAVEFORM_MODE_AUTO;
          break;
    }

    /* when we are in the useOnce-Field already the previous 
     * default-mode is already stored in the previous* vars
     * so don't overwrite it
     */
    if (!useModeOnce) {
	qDebug() << "saving previous mode";
        previousMode = currentMode;
        previousFlags = currentFlags;
	previousHalt = haltUpdates;
    }

    useModeOnce = justOnce;
    currentMode = newMode;
    currentFlags = newFlags;
    haltUpdates = (mode == MODE_EINK_BLOCK);
}

void EInkFbScreen::restoreRefreshMode()
{
	if(useModeOnce) {
		qDebug() << "going back to previous mode";
		currentMode = previousMode;
		currentFlags = previousFlags;
		haltUpdates = previousHalt;
		useModeOnce = 0;
	}
}

void EInkFbScreen::resetBlock()
{
	haltCount = 0;
	qDebug() << "resetBlock, haltcount now 0";
}

void EInkFbScreen::blockUpdates()
{
    haltCount++; 
    qDebug() << "block, haltcount now " << haltCount;
}

void EInkFbScreen::unblockUpdates()
{
    if(haltCount == 0)
        return;

    haltCount--;
    qDebug() << "unblock, haltcount now " << haltCount;
}

void EInkFbScreen::resetQueue()
{
	queueCount = 0;
	qDebug() << "resetQueue, count now 0";

	/* ignore pending regions */
	pendingPresent = false;
}

void EInkFbScreen::resetFlushQueue()
{
	queueCount = 0;
	qDebug() << "resetQueue, count now 0";

	/* flush pending updates to the screen */
	if (pendingPresent) {
		qDebug() << "flushing pending updates";
		setDirty(pendingArea);
		pendingPresent = false;
	}
}

void EInkFbScreen::queueUpdates()
{
	queueCount++;
	qDebug() << "queue updates, count now " << queueCount;
}

void EInkFbScreen::flushUpdates()
{
	/* mismatch, likely due to reset call */
	if(queueCount == 0)
		return;

	queueCount--;
	qDebug() << "flush updates, count now " << queueCount;
    
	if (queueCount == 0 && pendingPresent) {
		setDirty(pendingArea);
		pendingPresent = false;
	}
}

void EInkFbScreen::setDirty(const QRect& rect)
{
	bool waitComplete = false;

	qDebug() << "setDirty";
	QScreen::setDirty(rect);


	if (haltCount > 0) {
	    qDebug() << "haltCount > 0, ignoring updates";
	    return;
	}

	if (queueCount > 0) {
		if(pendingPresent)
			pendingArea = pendingArea.united(rect);
		else
			pendingArea = QRect(rect);
		pendingPresent = true;
		return;
	}

	if (currentFlags & FLAG_WAITFORCOMPLETION)
		waitComplete = true;

        if(!haltUpdates) {
	  if (currentFlags & FLAG_FULLSCREEN_UPDATE)
	    updateDisplay(0, 0, width(), height(), currentMode, waitComplete, 0, (currentFlags & FLAG_REFRESH));
	  else
            updateDisplay(rect.x(), rect.y(), rect.width(), rect.height(), currentMode, waitComplete, 0, (currentFlags & FLAG_REFRESH));
	  
	} else {
	  qDebug() << "ignoring update";
	}

        /* if we should only use the current mode once,
	 * return it to its previous settings.
	 */
	restoreRefreshMode();
	restoreUpdateScheme();
}

/* FIXME: this can probably go away completely */
void EInkFbScreen::exposeRegion(QRegion region, int changing)
{
    QScreen::exposeRegion(region, changing);
}

/*!
    \reimp

    This is called by \l{Qt for Embedded Linux} clients to map in the framebuffer.
    It should be reimplemented by accelerated drivers to map in
    graphics card registers; those drivers should then call this
    function in order to set up offscreen memory management. The
    device is specified in \a displaySpec; e.g. "/dev/fb".

    \sa disconnect()
*/

bool EInkFbScreen::connect(const QString &displaySpec)
{
    d_ptr->displaySpec = displaySpec;

    const QStringList args = displaySpec.split(QLatin1Char(':'));

    if (args.contains(QLatin1String("nographicsmodeswitch")))
        d_ptr->doGraphicsMode = false;

#ifdef QT_QWS_DEPTH_GENERIC
    if (args.contains(QLatin1String("genericcolors")))
        d_ptr->doGenericColors = true;
#endif

    QRegExp ttyRegExp(QLatin1String("tty=(.*)"));
    if (args.indexOf(ttyRegExp) != -1)
        d_ptr->ttyDevice = ttyRegExp.cap(1);

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#ifndef QT_QWS_FRAMEBUFFER_LITTLE_ENDIAN
    if (args.contains(QLatin1String("littleendian")))
#endif
        QScreen::setFrameBufferLittleEndian(true);
#endif

    // Check for explicitly specified device
    const int len = 8; // "/dev/fbx"
    int m = displaySpec.indexOf(QLatin1String("/dev/fb"));

    QString dev;
    if (m > 0)
        dev = displaySpec.mid(m, len);
    else
        dev = QLatin1String("/dev/fb0");

    if (access(dev.toLatin1().constData(), R_OK|W_OK) == 0)
        d_ptr->fd = open(dev.toLatin1().constData(), O_RDWR);

    if (d_ptr->fd == -1) {
        if (QApplication::type() == QApplication::GuiServer) {
            perror("QScreenLinuxFb::connect");
            qCritical("Error opening framebuffer device %s", qPrintable(dev));
            return false;
        }
        if (access(dev.toLatin1().constData(), R_OK) == 0)
            d_ptr->fd = open(dev.toLatin1().constData(), O_RDONLY);
    }

    setRotation(FB_ROTATE_CCW);

    /* normally we want to use the queue and merge scheme */
    setUpdateScheme(SCHEME_EINK_MERGE, false);

    fb_fix_screeninfo finfo;
    fb_var_screeninfo vinfo;

    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));

    /* Get fixed screen information */
    if (d_ptr->fd != -1 && ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("QEInkFbScreen::connect");
        qWarning("Error reading fixed information");
        return false;
    }

    if (finfo.type == FB_TYPE_VGA_PLANES) {
        qWarning("VGA16 video mode not supported");
        return false;
    }

    /* Get variable screen information */
    if (d_ptr->fd != -1 && ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("QEInkFbScreen::connect");
        qWarning("Error reading variable information");
        return false;
    }

	/* setup the display controller for QT to use */
	setupController();
	
    grayscale = vinfo.grayscale;
    d = vinfo.bits_per_pixel;
    if (d == 24) {
        d = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (d <= 0)
            d = 24; // reset if color component lengths are not reported
    } else if (d == 16) {
        d = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (d <= 0)
            d = 16;
    }
    lstep = finfo.line_length;

    int xoff = vinfo.xoffset;
    int yoff = vinfo.yoffset;
    const char* qwssize;
    if((qwssize=::getenv("QWS_SIZE")) && sscanf(qwssize,"%dx%d",&w,&h)==2) {
        if (d_ptr->fd != -1) {
            if ((uint)w > vinfo.xres) w = vinfo.xres;
            if ((uint)h > vinfo.yres) h = vinfo.yres;
        }
        dw=w;
        dh=h;
        int xxoff, yyoff;
        if (sscanf(qwssize, "%*dx%*d+%d+%d", &xxoff, &yyoff) == 2) {
            if (xxoff < 0 || xxoff + w > (int)vinfo.xres)
                xxoff = vinfo.xres - w;
            if (yyoff < 0 || yyoff + h > (int)vinfo.yres)
                yyoff = vinfo.yres - h;
            xoff += xxoff;
            yoff += yyoff;
        } else {
            xoff += (vinfo.xres - w)/2;
            yoff += (vinfo.yres - h)/2;
        }
    } else {
        dw=w=vinfo.xres;
        dh=h=vinfo.yres;
    }

    if (w == 0 || h == 0) {
        qWarning("QScreenEInkFb::connect(): Unable to find screen geometry, "
                 "will use 320x240.");
        dw = w = 320;
        dh = h = 240;
    }

    setPixelFormat(vinfo);

    // Handle display physical size spec.
    QStringList displayArgs = displaySpec.split(QLatin1Char(':'));
    QRegExp mmWidthRx(QLatin1String("mmWidth=?(\\d+)"));
    int dimIdxW = displayArgs.indexOf(mmWidthRx);
    QRegExp mmHeightRx(QLatin1String("mmHeight=?(\\d+)"));
    int dimIdxH = displayArgs.indexOf(mmHeightRx);
    if (dimIdxW >= 0) {
        mmWidthRx.exactMatch(displayArgs.at(dimIdxW));
        physWidth = mmWidthRx.cap(1).toInt();
        if (dimIdxH < 0)
            physHeight = dh*physWidth/dw;
    }
    if (dimIdxH >= 0) {
        mmHeightRx.exactMatch(displayArgs.at(dimIdxH));
        physHeight = mmHeightRx.cap(1).toInt();
        if (dimIdxW < 0)
            physWidth = dw*physHeight/dh;
    }
    if (dimIdxW < 0 && dimIdxH < 0) {
        if (vinfo.width != 0 && vinfo.height != 0
            && vinfo.width != UINT_MAX && vinfo.height != UINT_MAX) {
            physWidth = vinfo.width;
            physHeight = vinfo.height;
        } else {
            const int dpi = 72;
            physWidth = qRound(dw * 25.4 / dpi);
            physHeight = qRound(dh * 25.4 / dpi);
        }
    }

    dataoffset = yoff * lstep + xoff * d / 8;
    //qDebug("Using %dx%dx%d screen",w,h,d);

    /* Figure out the size of the screen in bytes */
    size = h * lstep;

    mapsize = finfo.smem_len;

    data = (unsigned char *)-1;
    if (d_ptr->fd != -1)
        data = (unsigned char *)mmap(0, mapsize, PROT_READ | PROT_WRITE,
                                     MAP_SHARED, d_ptr->fd, 0);

    if ((long)data == -1) {
        if (QApplication::type() == QApplication::GuiServer) {
            perror("QEInkFbScreen::connect");
            qWarning("Error: failed to map framebuffer device to memory.");
            return false;
        }
        data = 0;
    } else {
        data += dataoffset;
    }

    canaccel = useOffscreen();
    if(canaccel)
        setupOffScreen();

    // Now read in palette
    if((vinfo.bits_per_pixel==8) || (vinfo.bits_per_pixel==4)) {
        screencols= (vinfo.bits_per_pixel==8) ? 256 : 16;
        int loopc;
        fb_cmap startcmap;
        startcmap.start=0;
        startcmap.len=screencols;
        startcmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*screencols);
        startcmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*screencols);
        startcmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*screencols);
        startcmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*screencols);

        //Eason : always dropin this case
        if (d_ptr->fd == -1 || ioctl(d_ptr->fd, FBIOGETCMAP, &startcmap)) {
            perror("EInkFbScreen::connect");
            qWarning("Error reading palette from framebuffer, using default palette");
            createPalette(startcmap, vinfo, finfo);
        }
        int bits_used = 0;
        for(loopc=0;loopc<screencols;loopc++) {
            screenclut[loopc]=qRgb(startcmap.red[loopc] >> 8,
                                   startcmap.green[loopc] >> 8,
                                   startcmap.blue[loopc] >> 8);
            bits_used |= startcmap.red[loopc]
                         | startcmap.green[loopc]
                         | startcmap.blue[loopc];
        }
        // WORKAROUND: Some framebuffer drivers only return 8 bit
        // color values, so we need to not bit shift them..
        if ((bits_used & 0x00ff) && !(bits_used & 0xff00)) {
            for(loopc=0;loopc<screencols;loopc++) {
                screenclut[loopc] = qRgb(startcmap.red[loopc],
                                         startcmap.green[loopc],
                                         startcmap.blue[loopc]);
            }
            qWarning("8 bits cmap returned due to faulty FB driver, colors corrected");
        }
        free(startcmap.red);
        free(startcmap.green);
        free(startcmap.blue);
        free(startcmap.transp);
    } else {
        screencols=0;
    }

    return true;
}

/*!
    \reimp

    This unmaps the framebuffer.

    \sa connect()
*/

void EInkFbScreen::disconnect()
{
    data -= dataoffset;
	
	// restore the state of the display controller
	restoreController();
	
    if (data)
        munmap((char*)data,mapsize);
    close(d_ptr->fd);
}

 #define DEBUG_VINFO

void EInkFbScreen::createPalette(fb_cmap &cmap, fb_var_screeninfo &vinfo, fb_fix_screeninfo &finfo)
{
    if((vinfo.bits_per_pixel==8) || (vinfo.bits_per_pixel==4)) {
        screencols= (vinfo.bits_per_pixel==8) ? 256 : 16;
        cmap.start=0;
        cmap.len=screencols;
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*screencols);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*screencols);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*screencols);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*screencols);

        if (screencols==16) {
            if (finfo.type == FB_TYPE_PACKED_PIXELS) {
                // We'll setup a grayscale cmap for 4bpp linear
                int val = 0;
                for (int idx = 0; idx < 16; ++idx, val += 17) {
                    cmap.red[idx] = (val<<8)|val;
                    cmap.green[idx] = (val<<8)|val;
                    cmap.blue[idx] = (val<<8)|val;
                    screenclut[idx]=qRgb(val, val, val);
                     qDebug("ES:%s:create color index(%d)\n", __func__, idx);
                }
            } else {
                // Default 16 colour palette
                // Green is now trolltech green so certain images look nicer
                //                             black  d_gray l_gray white  red  green  blue cyan magenta yellow
                unsigned char reds[16]   = { 0x00, 0x7F, 0xBF, 0xFF, 0xFF, 0xA2, 0x00, 0xFF, 0xFF, 0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x82 };
                unsigned char greens[16] = { 0x00, 0x7F, 0xBF, 0xFF, 0x00, 0xC5, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x7F };
                unsigned char blues[16]  = { 0x00, 0x7F, 0xBF, 0xFF, 0x00, 0x11, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x7F, 0x7F, 0x7F, 0x00, 0x00 };

                for (int idx = 0; idx < 16; ++idx) {
                    cmap.red[idx] = ((reds[idx]) << 8)|reds[idx];
                    cmap.green[idx] = ((greens[idx]) << 8)|greens[idx];
                    cmap.blue[idx] = ((blues[idx]) << 8)|blues[idx];
                    cmap.transp[idx] = 0;
                    screenclut[idx]=qRgb(reds[idx], greens[idx], blues[idx]);
                }
            }
        } else {
            if (grayscale) {
                // Build grayscale palette
                int i;
                for(i=0;i<screencols;++i) {
                    int bval = screencols == 256 ? i : (i << 4);
                    ushort val = (bval << 8) | bval;
                    cmap.red[i] = val;
                    cmap.green[i] = val;
                    cmap.blue[i] = val;
                    cmap.transp[i] = 0;
                    screenclut[i] = qRgb(bval,bval,bval);
                }
            } else {
                // 6x6x6 216 color cube
                int idx = 0;
                for(int ir = 0x0; ir <= 0xff; ir+=0x33) {
                    for(int ig = 0x0; ig <= 0xff; ig+=0x33) {
                        for(int ib = 0x0; ib <= 0xff; ib+=0x33) {
                            cmap.red[idx] = (ir << 8)|ir;
                            cmap.green[idx] = (ig << 8)|ig;
                            cmap.blue[idx] = (ib << 8)|ib;
                            cmap.transp[idx] = 0;
                            screenclut[idx]=qRgb(ir, ig, ib);
                            ++idx;
                        }
                    }
                }
                // Fill in rest with 0
                for (int loopc=0; loopc<40; ++loopc) {
                    screenclut[idx]=0;
                    ++idx;
                }
                screencols=idx;
            }
        }
    } else if(finfo.visual==FB_VISUAL_DIRECTCOLOR) {
        cmap.start=0;
        int rbits=0,gbits=0,bbits=0;
        switch (vinfo.bits_per_pixel) {
        case 8:
            rbits=vinfo.red.length;
            gbits=vinfo.green.length;
            bbits=vinfo.blue.length;
            if(rbits==0 && gbits==0 && bbits==0) {
                // cyber2000 driver bug hack
                rbits=3;
                gbits=3;
                bbits=2;
            }
            break;
        case 15:
            rbits=5;
            gbits=5;
            bbits=5;
            break;
        case 16:
            rbits=5;
            gbits=6;
            bbits=5;
            break;
        case 18:
        case 19:
            rbits=6;
            gbits=6;
            bbits=6;
            break;
        case 24: case 32:
            rbits=gbits=bbits=8;
            break;
        }
        screencols=cmap.len=1<<qMax(rbits,qMax(gbits,bbits));
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*256);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*256);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*256);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*256);
        for(unsigned int i = 0x0; i < cmap.len; i++) {
            cmap.red[i] = i*65535/((1<<rbits)-1);
            cmap.green[i] = i*65535/((1<<gbits)-1);
            cmap.blue[i] = i*65535/((1<<bbits)-1);
            cmap.transp[i] = 0;
        }
    }
}

/*!
    \reimp

    This is called by the \l{Qt for Embedded Linux} server at startup time.
    It turns off console blinking, sets up the color palette, enables write
    combining on the framebuffer and initialises the off-screen memory
    manager.
*/

bool EInkFbScreen::initDevice()
{
    d_ptr->openTty();

    // Grab current mode so we can reset it
    fb_var_screeninfo vinfo;
    fb_fix_screeninfo finfo;
    //#######################
    // Shut up Valgrind
    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));
    //#######################

    if (ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("EInkFbScreen::initDevice");
        qFatal("Error reading variable information in card init");
        return false;
    }

#ifdef DEBUG_VINFO
    qDebug("Greyscale %d",vinfo.grayscale);
    qDebug("Nonstd %d",vinfo.nonstd);
    qDebug("Red %d %d %d",vinfo.red.offset,vinfo.red.length,
           vinfo.red.msb_right);
    qDebug("Green %d %d %d",vinfo.green.offset,vinfo.green.length,
           vinfo.green.msb_right);
    qDebug("Blue %d %d %d",vinfo.blue.offset,vinfo.blue.length,
           vinfo.blue.msb_right);
    qDebug("Transparent %d %d %d",vinfo.transp.offset,vinfo.transp.length,
           vinfo.transp.msb_right);
#endif

    d_ptr->startupw=vinfo.xres;
    d_ptr->startuph=vinfo.yres;
    d_ptr->startupd=vinfo.bits_per_pixel;
    grayscale = vinfo.grayscale;

    if (ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("EInkFbScreen::initDevice");
        qCritical("Error reading fixed information in card init");
        // It's not an /error/ as such, though definitely a bad sign
        // so we return true
        return true;
    }

    if ((vinfo.bits_per_pixel==8) || (vinfo.bits_per_pixel==4) || (finfo.visual==FB_VISUAL_DIRECTCOLOR))
    {
        fb_cmap cmap;
        createPalette(cmap, vinfo, finfo);
        if (ioctl(d_ptr->fd, FBIOPUTCMAP, &cmap)) {
            perror("EInkFbScreen::initDevice");
            qWarning("Error writing palette to framebuffer");
        }
        free(cmap.red);
        free(cmap.green);
        free(cmap.blue);
        free(cmap.transp);
    }

    if (canaccel && entryp) {

qDebug() << __func__ << " cache";
        *entryp=0;
        *lowest = mapsize;
        insert_entry(*entryp, *lowest, *lowest);  // dummy entry to mark start
    }

    shared->fifocount = 0;
    shared->buffer_offset = 0xffffffff;  // 0 would be a sensible offset (screen)
    shared->linestep = 0;
    shared->cliptop = 0xffffffff;
    shared->clipleft = 0xffffffff;
    shared->clipright = 0xffffffff;
    shared->clipbottom = 0xffffffff;
    shared->rop = 0xffffffff;

#ifdef QT_QWS_DEPTH_GENERIC
    if (pixelFormat() == QImage::Format_Invalid && screencols == 0
        && d_ptr->doGenericColors)
    {
        qt_set_generic_blit(this, vinfo.bits_per_pixel,
                            vinfo.red.length, vinfo.green.length,
                            vinfo.blue.length, vinfo.transp.length,
                            vinfo.red.offset, vinfo.green.offset,
                            vinfo.blue.offset, vinfo.transp.offset);
    }
#endif
// Eason for hide cursor directly 20100408
#ifndef QT_NO_QWS_CURSOR
//    QScreenCursor::initSoftwareCursor();
#endif
    blank(false);

    return true;
}

/*
  The offscreen memory manager's list of entries is stored at the bottom
  of the offscreen memory area and consistes of a series of QPoolEntry's,
  each of which keep track of a block of allocated memory. Unallocated memory
  is implicitly indicated by the gap between blocks indicated by QPoolEntry's.
  The memory manager looks through any unallocated memory before the end
  of currently-allocated memory to see if a new block will fit in the gap;
  if it doesn't it allocated it from the end of currently-allocated memory.
  Memory is allocated from the top of the framebuffer downwards; if it hits
  the list of entries then offscreen memory is full and further allocations
  are made from main RAM (and hence unaccelerated). Allocated memory can
  be seen as a sort of upside-down stack; lowest keeps track of the
  bottom of the stack.
*/

void EInkFbScreen::delete_entry(int pos)
{

qDebug() << __func__;
    if (pos > *entryp || pos < 0) {
        qWarning("Attempt to delete odd pos! %d %d", pos, *entryp);
        return;
    }

#ifdef DEBUG_CACHE
    qDebug("Remove entry: %d", pos);
#endif

    QPoolEntry *qpe = &entries[pos];
    if (qpe->start <= *lowest) {
        // Lowest goes up again
        *lowest = entries[pos-1].start;
#ifdef DEBUG_CACHE
        qDebug("   moved lowest to %d", *lowest);
#endif
    }

    (*entryp)--;
    if (pos == *entryp)
        return;

    int size = (*entryp)-pos;
    memmove(&entries[pos], &entries[pos+1], size*sizeof(QPoolEntry));
}

void EInkFbScreen::insert_entry(int pos, int start, int end)
{

qDebug() << __func__;
    if (pos > *entryp) {
        qWarning("Attempt to insert odd pos! %d %d",pos,*entryp);
        return;
    }

#ifdef DEBUG_CACHE
    qDebug("Insert entry: %d, %d -> %d", pos, start, end);
#endif

    if (start < (int)*lowest) {
        *lowest = start;
#ifdef DEBUG_CACHE
        qDebug("    moved lowest to %d", *lowest);
#endif
    }

    if (pos == *entryp) {
        entries[pos].start = start;
        entries[pos].end = end;
        entries[pos].clientId = qws_client_id;
        (*entryp)++;
        return;
    }

    int size=(*entryp)-pos;
    memmove(&entries[pos+1],&entries[pos],size*sizeof(QPoolEntry));
    entries[pos].start=start;
    entries[pos].end=end;
    entries[pos].clientId=qws_client_id;
    (*entryp)++;
}

/*!
    \fn uchar * EInkFbScreen::cache(int amount)

    Requests the specified \a amount of offscreen graphics card memory
    from the memory manager, and returns a pointer to the data within
    the framebuffer (or 0 if there is no free memory).

    Note that the display is locked while memory is allocated in order to
    preserve the memory pool's integrity.

    Use the QScreen::onCard() function to retrieve an offset (in
    bytes) from the start of graphics card memory for the returned
    pointer.

    \sa uncache(), clearCache(), deleteEntry()
*/

uchar * EInkFbScreen::cache(int amount)
{
    if (!canaccel || entryp == 0)
        return 0;

    qt_fbdpy->grab();

    int startp = cacheStart + (*entryp+1) * sizeof(QPoolEntry);
    if (startp >= (int)*lowest) {
        // We don't have room for another cache QPoolEntry.
#ifdef DEBUG_CACHE
        qDebug("No room for pool entry in VRAM");
#endif
        qt_fbdpy->ungrab();
        return 0;
    }

    int align = pixmapOffsetAlignment();

    if (*entryp > 1) {
        // Try to find a gap in the allocated blocks.
        for (int loopc = 0; loopc < *entryp-1; loopc++) {
            int freestart = entries[loopc+1].end;
            int freeend = entries[loopc].start;
            if (freestart != freeend) {
                while (freestart % align) {
                    freestart++;
                }
                int len=freeend-freestart;
                if (len >= amount) {
                    insert_entry(loopc+1, freestart, freestart+amount);
                    qt_fbdpy->ungrab();
                    return data+freestart;
                }
            }
        }
    }

    // No free blocks in already-taken memory; get some more
    // if we can
    int newlowest = (*lowest)-amount;
    if (newlowest % align) {
        newlowest -= align;
        while (newlowest % align) {
            newlowest++;
        }
    }
    if (startp >= newlowest) {
        qt_fbdpy->ungrab();
#ifdef DEBUG_CACHE
        qDebug("No VRAM available for %d bytes", amount);
#endif
        return 0;
    }
    insert_entry(*entryp, newlowest, *lowest);
    qt_fbdpy->ungrab();

    return data + newlowest;
}

/*!
    \fn void EInkFbScreen::uncache(uchar * memoryBlock)

    Deletes the specified \a memoryBlock allocated from the graphics
    card memory.

    Note that the display is locked while memory is unallocated in
    order to preserve the memory pool's integrity.

    This function will first sync the graphics card to ensure the
    memory isn't still being used by a command in the graphics card
    FIFO queue. It is possible to speed up a driver by overriding this
    function to avoid syncing. For example, the driver might delay
    deleting the memory until it detects that all commands dealing
    with the memory are no longer in the queue. Note that it will then
    be up to the driver to ensure that the specified \a memoryBlock no
    longer is being used.

    \sa cache(), deleteEntry(), clearCache()
 */
void EInkFbScreen::uncache(uchar * c)
{
    // need to sync graphics card

qDebug() << __func__;

    deleteEntry(c);
}

/*!
    \fn void EInkFbScreen::deleteEntry(uchar * memoryBlock)

    Deletes the specified \a memoryBlock allocated from the graphics
    card memory.

    \sa uncache(), cache(), clearCache()
*/
void EInkFbScreen::deleteEntry(uchar * c)
{

qDebug() << __func__;
    qt_fbdpy->grab();
    unsigned long pos=(unsigned long)c;
    pos-=((unsigned long)data);
    unsigned int hold=(*entryp);
    for(unsigned int loopc=1;loopc<hold;loopc++) {
        if (entries[loopc].start==pos) {
            if (entries[loopc].clientId == qws_client_id)
                delete_entry(loopc);
            else
                qWarning("Attempt to delete client id %d cache entry",
                         entries[loopc].clientId);
            qt_fbdpy->ungrab();
            return;
        }
    }
    qt_fbdpy->ungrab();
    qWarning("Attempt to delete unknown offset %ld",pos);
}

/*!
    Removes all entries from the cache for the specified screen \a
    instance and client identified by the given \a clientId.

    Calling this function should only be necessary if a client exits
    abnormally.

    \sa cache(), uncache(), deleteEntry()
*/
void EInkFbScreen::clearCache(QScreen *instance, int clientId)
{
    EInkFbScreen *screen = (EInkFbScreen *)instance;

qDebug() << __func__;
    if (!screen->canaccel || !screen->entryp)
        return;
    qt_fbdpy->grab();
    for (int loopc = 0; loopc < *(screen->entryp); loopc++) {
        if (screen->entries[loopc].clientId == clientId) {
            screen->delete_entry(loopc);
            loopc--;
        }
    }
    qt_fbdpy->ungrab();
}


void EInkFbScreen::setupOffScreen()
{
    // Figure out position of offscreen memory
    // Set up pool entries pointer table and 64-bit align it
    int psize = size;

qDebug() << __func__;

    // hw: this causes the limitation of cursors to 64x64
    // the cursor should rather use the normal pixmap mechanism
    psize += 4096;  // cursor data
    psize += 8;     // for alignment
    psize &= ~0x7;  // align

    unsigned long pos = (unsigned long)data;
    pos += psize;
    entryp = ((int *)pos);
    lowest = ((unsigned int *)pos)+1;
    pos += (sizeof(int))*4;
    entries = (QPoolEntry *)pos;

    // beginning of offscreen memory available for pixmaps.
    cacheStart = psize + 4*sizeof(int) + sizeof(QPoolEntry);
}

/*!
    \reimp

    This is called by the \l{Qt for Embedded Linux} server when it shuts
    down, and should be inherited if you need to do any card-specific cleanup.
    The default version hides the screen cursor and reenables the blinking
    cursor and screen blanking.
*/

void EInkFbScreen::shutdownDevice()
{
    // Causing crashes. Not needed.
    //setMode(startupw,startuph,startupd);
/*
    if (startupd == 8) {
        ioctl(fd,FBIOPUTCMAP,startcmap);
        free(startcmap->red);
        free(startcmap->green);
        free(startcmap->blue);
        free(startcmap->transp);
        delete startcmap;
        startcmap = 0;
    }
*/
    d_ptr->closeTty();
}

/*!
    \fn void EInkFbScreen::set(unsigned int index,unsigned int red,unsigned int green,unsigned int blue)

    Sets the specified color \a index to the specified RGB value, (\a
    red, \a green, \a blue), when in paletted graphics modes.
*/

void EInkFbScreen::set(unsigned int i,unsigned int r,unsigned int g,unsigned int b)
{
    if (d_ptr->fd != -1) {
        fb_cmap cmap;
        cmap.start=i;
        cmap.len=1;
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*256);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*256);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*256);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*256);
        cmap.red[0]=r << 8;
        cmap.green[0]=g << 8;
        cmap.blue[0]=b << 8;
        cmap.transp[0]=0;
        ioctl(d_ptr->fd, FBIOPUTCMAP, &cmap);
        free(cmap.red);
        free(cmap.green);
        free(cmap.blue);
        free(cmap.transp);
    }
    screenclut[i] = qRgb(r, g, b);
}

/*!
    \reimp

    Sets the framebuffer to a new resolution and bit depth. The width is
    in \a nw, the height is in \a nh, and the depth is in \a nd. After
    doing this any currently-existing paint engines will be invalid and the
    screen should be completely redrawn. In a multiple-process
    Embedded Qt situation you must signal all other applications to
    call setMode() to the same mode and redraw.
*/

void EInkFbScreen::setMode(int nw,int nh,int nd)
{

    printf("[DBG] %s,%s,%d, nw=%d,nh=%d,nd=%d", __FILE__, __func__, __LINE__, nw, nh, nd);
    if (d_ptr->fd == -1)
        return;

    fb_fix_screeninfo finfo;
    fb_var_screeninfo vinfo;
    //#######################
    // Shut up Valgrind
    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));
    //#######################

    if (ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("EInkFbScreen::setMode");
        qFatal("Error reading variable information in mode change");
    }

    vinfo.xres=nw;
    vinfo.yres=nh;

    vinfo.bits_per_pixel=nd;
    
    /* insert hardcoded rotation for now */
//    vinfo.rotate = FB_ROTATE_CCW;

    if (ioctl(d_ptr->fd, FBIOPUT_VSCREENINFO, &vinfo)) {
        perror("EInkFbScreen::setMode");
        qCritical("Error writing variable information in mode change");
    }

    if (ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("EInkFbScreen::setMode");
        qFatal("Error reading changed variable information in mode change");
    }
qDebug() << __func__ << "got rotation " << vinfo.rotate;

    if (ioctl(d_ptr->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("EInkFbScreen::setMode");
        qFatal("Error reading fixed information");
    }

    disconnect();
    connect(d_ptr->displaySpec);
    exposeRegion(region(), 0);
}

// save the state of the graphics card
// This is needed so that e.g. we can restore the palette when switching
// between linux virtual consoles.

/*!
    \reimp

    This doesn't do anything; accelerated drivers may wish to reimplement
    it to save graphics cards registers. It's called by the
    \l{Qt for Embedded Linux} server when the virtual console is switched.
*/

void EInkFbScreen::save()
{
    // nothing to do.
}


// restore the state of the graphics card.
/*!
    \reimp

    This is called when the virtual console is switched back to
    \l{Qt for Embedded Linux} and restores the palette.
*/
void EInkFbScreen::restore()
{
    if (d_ptr->fd == -1)
        return;

    if ((d == 8) || (d == 4)) {
        fb_cmap cmap;
        cmap.start=0;
        cmap.len=screencols;
        cmap.red=(unsigned short int *)
                 malloc(sizeof(unsigned short int)*256);
        cmap.green=(unsigned short int *)
                   malloc(sizeof(unsigned short int)*256);
        cmap.blue=(unsigned short int *)
                  malloc(sizeof(unsigned short int)*256);
        cmap.transp=(unsigned short int *)
                    malloc(sizeof(unsigned short int)*256);
        for (int loopc = 0; loopc < screencols; loopc++) {
            cmap.red[loopc] = qRed(screenclut[loopc]) << 8;
            cmap.green[loopc] = qGreen(screenclut[loopc]) << 8;
            cmap.blue[loopc] = qBlue(screenclut[loopc]) << 8;
            cmap.transp[loopc] = 0;
        }
        ioctl(d_ptr->fd, FBIOPUTCMAP, &cmap);
        free(cmap.red);
        free(cmap.green);
        free(cmap.blue);
        free(cmap.transp);
    }
}

/*!
    \fn int EInkFbScreen::sharedRamSize(void * end)
    \internal
*/

// This works like the QScreenCursor code. end points to the end
// of our shared structure, we return the amount of memory we reserved
int EInkFbScreen::sharedRamSize(void * end)
{
    shared=(EInkFb_Shared *)end;
    shared--;
    return sizeof(EInkFb_Shared);
}

/*!
    \reimp
*/
void EInkFbScreen::blank(bool on)
{
    if (d_ptr->blank == on)
        return;

#if defined(QT_QWS_IPAQ)
    if (on)
        system("apm -suspend");
#else
    if (d_ptr->fd == -1)
        return;
// Some old kernel versions don't have this.  These defines should go
// away eventually
#if defined(FBIOBLANK)
#if defined(VESA_POWERDOWN) && defined(VESA_NO_BLANKING)
    ioctl(d_ptr->fd, FBIOBLANK, on ? VESA_POWERDOWN : VESA_NO_BLANKING);
#else
    ioctl(d_ptr->fd, FBIOBLANK, on ? 1 : 0);
#endif
#endif
#endif

    d_ptr->blank = on;
}

/*
 *
 * TODO: ES: Add dither here is a good way
 */
void EInkFbScreen::setPixelFormat(struct fb_var_screeninfo info)
{
    const fb_bitfield rgba[4] = { info.red, info.green,
                                  info.blue, info.transp };

    QImage::Format format = QImage::Format_Invalid;

    printf("%s, d=%d\n", __func__, d);
    switch (d) {
    case 32: {
        const fb_bitfield argb8888[4] = {{16, 8, 0}, {8, 8, 0},
                                         {0, 8, 0}, {24, 8, 0}};
        const fb_bitfield abgr8888[4] = {{0, 8, 0}, {8, 8, 0},
                                         {16, 8, 0}, {24, 8, 0}};
        if (memcmp(rgba, argb8888, 4 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_ARGB32;
        } else if (memcmp(rgba, argb8888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB32;
        } else if (memcmp(rgba, abgr8888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB32;
            pixeltype = QScreen::BGRPixel;
        }
        break;
    }
    case 24: {
        const fb_bitfield rgb888[4] = {{16, 8, 0}, {8, 8, 0},
                                       {0, 8, 0}, {0, 0, 0}};
        const fb_bitfield bgr888[4] = {{0, 8, 0}, {8, 8, 0},
                                       {16, 8, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB888;
        } else if (memcmp(rgba, bgr888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB888;
            pixeltype = QScreen::BGRPixel;
        }
        break;
    }
    case 18: {
        const fb_bitfield rgb666[4] = {{12, 6, 0}, {6, 6, 0},
                                       {0, 6, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb666, 3 * sizeof(fb_bitfield)) == 0)
            format = QImage::Format_RGB666;
        break;
    }
    case 16: {
        const fb_bitfield rgb565[4] = {{11, 5, 0}, {5, 6, 0},
                                       {0, 5, 0}, {0, 0, 0}};
        const fb_bitfield bgr565[4] = {{0, 5, 0}, {5, 6, 0},
                                       {11, 5, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb565, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB16;
        } else if (memcmp(rgba, bgr565, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB16;
            pixeltype = QScreen::BGRPixel;
        }
        break;
    }
    case 15: {
        const fb_bitfield rgb1555[4] = {{10, 5, 0}, {5, 5, 0},
                                        {0, 5, 0}, {15, 1, 0}};
        const fb_bitfield bgr1555[4] = {{0, 5, 0}, {5, 5, 0},
                                        {10, 5, 0}, {15, 1, 0}};
        if (memcmp(rgba, rgb1555, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB555;
        } else if (memcmp(rgba, bgr1555, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB555;
            pixeltype = QScreen::BGRPixel;
        }
        break;
    }
    case 12: {
        const fb_bitfield rgb444[4] = {{8, 4, 0}, {4, 4, 0},
                                       {0, 4, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb444, 3 * sizeof(fb_bitfield)) == 0)
            format = QImage::Format_RGB444;
        break;
    }
    case 8:
    	break;
    case 1:
        format = QImage::Format_Mono; //###: LSB???
        break;
    default:
        break;
    }

    QScreen::setPixelFormat(format);
}

bool EInkFbScreen::useOffscreen()
{
    if ((mapsize - size) < 16*1024)
        return false;

    return true;
}

void EInkFbScreen::setupController()
{
}

void EInkFbScreen::restoreController()
{
}

int EInkFbScreen::updateDisplay(int left, int top, int width, int height, int wave_mode,
    int wait_for_complete, uint flags, int fullUpdates)
{
    struct mxcfb_update_data upd_data;
    int retval;

    upd_data.update_mode = fullUpdates ? UPDATE_MODE_FULL : UPDATE_MODE_PARTIAL;
    upd_data.waveform_mode = wave_mode;
    upd_data.update_region.left = left;
    upd_data.update_region.width = width;
    upd_data.update_region.top = top;
    upd_data.update_region.height = height;
    upd_data.temp = TEMP_USE_AMBIENT;
    upd_data.flags = flags;

    if (wait_for_complete) {
        /* Get unique marker value */
        upd_data.update_marker = marker_val++;
    } else {
        upd_data.update_marker = 0;
    }

    qDebug("send update (x,y,w,h,waveform,mode,flags) with (%d, %d, %d, %d, %d, %d, %d)\n", 
       upd_data.update_region.left, upd_data.update_region.top,
       upd_data.update_region.width, upd_data.update_region.height,
       wave_mode,  upd_data.update_mode, upd_data.flags);
    retval = ioctl(d_ptr->fd, MXCFB_SEND_UPDATE, &upd_data);
    while (retval < 0) {
        /* We have limited memory available for updates, so wait and
         * then try again after some updates have completed */
        qDebug("send_update returned %d, retrying\n", retval);
        sleep(1);
        retval = ioctl(d_ptr->fd, MXCFB_SEND_UPDATE, &upd_data);
    }

    if (wait_for_complete) {
        qDebug("waiting for update to complete\n");

        /* Wait for update to complete */
        retval = ioctl(d_ptr->fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &upd_data.update_marker);
        if (retval < 0) {
            printf("Wait for update complete failed.  Error = 0x%x\n", retval);
        }
    }

    return 0;
}

int EInkFbScreen::setRotation(int rot)
{
        int retval;
        struct fb_var_screeninfo screen_info;

        memset(&screen_info, 0, sizeof(screen_info));

        if (ioctl(d_ptr->fd, FBIOGET_VSCREENINFO, &screen_info)) {
            qWarning("Error reading variable information");
            return false;
        }

        qDebug() << "Rotating FB to " << rot;
        screen_info.rotate = rot;

        /* it seems 16bit color, results in the best visual result.
	 * 8bit grayscale processed by the pxp results in a lot of ghosting
	 */
        screen_info.bits_per_pixel = 16;
        screen_info.grayscale = 0;
        //screen_info.bits_per_pixel = 8;
        //screen_info.grayscale = 1;

        retval = ioctl(d_ptr->fd, FBIOPUT_VSCREENINFO, &screen_info);
        if (retval < 0)
        {
                printf("Rotation failed\n");
                return(-1);
        }

        qDebug("New dimensions: xres = %d, xres_virtual = %d,"
                "yres = %d, yres_virtual = %d\n",
                screen_info.xres, screen_info.xres_virtual,
                screen_info.yres, screen_info.yres_virtual);
        return(0);
}

void EInkFbScreen::setUpdateScheme(int newScheme, bool justOnce)
{
    int retval;
    unsigned long scheme;
 
    switch(newScheme) {
      case SCHEME_EINK_QUEUE:
	scheme = UPDATE_SCHEME_QUEUE;
	break;
      case SCHEME_EINK_MERGE:
      default:
	scheme = UPDATE_SCHEME_QUEUE_AND_MERGE;
	break;
    }

    qDebug() << "setting update scheme to " << scheme;
    retval = ioctl(d_ptr->fd, MXCFB_SET_UPDATE_SCHEME, &scheme);
    if (retval < 0)
    {
            printf("setting scheme failed\n");
            return;
    }

    previousScheme = currentScheme;
    currentScheme = scheme;
    useSchemeOnce = justOnce;

    return;    
}

void EInkFbScreen::restoreUpdateScheme()
{
	if(useSchemeOnce) {
		qDebug() << "going back to previous scheme";
		currentScheme = previousScheme;
		useSchemeOnce = 0;
	}
}

QT_END_NAMESPACE

#endif // QT_NO_QWS_EINKFB
