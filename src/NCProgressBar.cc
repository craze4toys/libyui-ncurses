/*---------------------------------------------------------------------\
|                                                                      |
|                      __   __    ____ _____ ____                      |
|                      \ \ / /_ _/ ___|_   _|___ \                     |
|                       \ V / _` \___ \ | |   __) |                    |
|                        | | (_| |___) || |  / __/                     |
|                        |_|\__,_|____/ |_| |_____|                    |
|                                                                      |
|                               core system                            |
|                                                        (C) SuSE GmbH |
\----------------------------------------------------------------------/

   File:       NCProgressBar.cc

   Author:     Michael Andres <ma@suse.de>
   Maintainer: Michael Andres <ma@suse.de>

/-*/
#include "Y2Log.h"
#include "NCurses.h"
#include "NCProgressBar.h"

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::NCProgressBar
//	METHOD TYPE : Constructor
//
//	DESCRIPTION :
//
NCProgressBar::NCProgressBar( NCWidget * parent, YWidgetOpt & opt,
			      const YCPString & nlabel,
			      const YCPInteger & maxprogress,
			      const YCPInteger & progress )
    : YProgressBar( opt, nlabel, maxprogress, progress )
    , NCWidget( parent )
    , label( nlabel )
    , maxval( maxprogress->value() )
    , cval( progress->value() )
    , lwin( 0 )
    , twin( 0 )
{
  WIDDBG << endl;
  if ( maxval <= 0 )
    maxval = 1;
  hotlabel = &label;
  setLabel( nlabel );
  setProgress( progress );
  wstate = NC::WSdumb;
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::~NCProgressBar
//	METHOD TYPE : Destructor
//
//	DESCRIPTION :
//
NCProgressBar::~NCProgressBar()
{
  delete lwin;
  delete twin;
  WIDDBG << endl;
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::nicesize
//	METHOD TYPE : long
//
//	DESCRIPTION :
//
long NCProgressBar::nicesize( YUIDimension dim )
{
  return dim == YD_HORIZ ? wGetDefsze().W : wGetDefsze().H;
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::setSize
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::setSize( long newwidth, long newheight )
{
  wRelocate( wpos( 0 ), wsze( newheight, newwidth ) );
  YProgressBar::setSize( newwidth, newheight );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::setDefsze
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::setDefsze()
{
  defsze = wsze( label.height() + 1,
		 label.width() < 5 ? 5 : label.width() );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::wCreate
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::wCreate( const wrect & newrect )
{
  NCWidget::wCreate( newrect );

  wrect lrect( 0, wsze::min( newrect.Sze,
			     wsze( label.height(), newrect.Sze.W ) ) );
  wrect trect( 0, wsze( 1, newrect.Sze.W ) );

  if ( lrect.Sze.H == newrect.Sze.H )
    lrect.Sze.H -= 1;

  trect.Pos.L = lrect.Sze.H > 0 ? lrect.Sze.H : 0;

  lwin = new NCursesWindow( *win,
			    lrect.Sze.H, lrect.Sze.W,
			    lrect.Pos.L, lrect.Pos.C,
			    'r' );
  twin = new NCursesWindow( *win,
			    trect.Sze.H, trect.Sze.W,
			    trect.Pos.L, trect.Pos.C,
			    'r' );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::wDelete
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::wDelete()
{
  delete lwin;
  delete twin;
  lwin = 0;
  twin = 0;
  NCWidget::wDelete();
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::setLabel
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::setLabel( const YCPString & nlabel )
{
  label = NCstring( nlabel );
  setDefsze();
  YProgressBar::setLabel( nlabel );
  Redraw();
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::setProgress
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::setProgress( const YCPInteger & nval )
{
  cval = nval->value();
  if ( cval < 0 )
    cval = 0;
  else if ( cval > maxval )
    cval = maxval;
  Redraw();
  YProgressBar::setProgress( nval );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::wRedraw
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::wRedraw()
{
  if ( !win )
    return;

  // label
  chtype bg = wStyle().dumb.text;
  lwin->bkgdset( bg );
  lwin->clear();
  label.drawAt( *lwin, bg, bg );
  tUpdate();
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCProgressBar::tUpdate
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCProgressBar::tUpdate()
{
  if ( !win )
    return;

  double split = double(twin->maxx()+1) * cval / maxval;
  int cp = int(split);
  if ( cp == 0 && split > 0.0 )
    cp = 1;

  const NCstyle::StProgbar & style( wStyle().progbar );
  twin->bkgdset( style.bar.chattr );
  twin->clear();

  if ( cp <= twin->maxx() ) {
    twin->bkgdset( style.nonbar.chattr );
    twin->printw( 0, cp, "%*s", twin->width()-cp, "" );
  }

  if ( twin->maxx() >= 6 ) {
    int pc  = 100 * cval / maxval;
    int off = twin->maxx() / 2 - ( pc == 100 ? 2
					     : pc >= 10 ? 1
							: 0 );
    char buf[5];
    sprintf( buf, "%d%%", pc );
    twin->move( 0, off );
    for ( char * ch = buf; *ch; ++ch ) {
      chtype a = twin->inch();
      NCattribute::setChar( a, *ch );
      twin->addch( a );
    }
  }
}
