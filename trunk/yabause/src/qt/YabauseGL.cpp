/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "YabauseGL.h"
#include "QtYabause.h"

YabauseGL::YabauseGL( QWidget* p )
	: QGLWidget( p )
{}

void YabauseGL::showEvent( QShowEvent* e )
{
	// hack for clearing the the gl context
	QGLWidget::showEvent( e );
	QSize s = size();
	resize( 0, 0 );
	resize( s );
}

void YabauseGL::resizeGL( int w, int h )
{
	glViewport( 0, 0, w, h );
	if ( VIDCore )
		VIDCore->Resize( w, h, 0 );
}
