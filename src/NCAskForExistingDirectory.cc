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

   File:       NCAskForExistingDirectory.cc

   Author:     Gabriele Strattner <gs@suse.de>
   Maintainer: Michael Andres <ma@suse.de>

/-*/
#include "Y2Log.h"

#include "NCAskForExistingDirectory.h"

#include "YDialog.h"
#include "NCSplit.h"
#include "NCSpacing.h"
#include "PkgNames.h"
#include "NCFrame.h"

#include <Y2PM.h>
#include <y2pm/RpmDb.h>
#include <y2pm/PMSelectionManager.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/errno.h>


///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCAskForExistingDirectory::NCAskForExistingDirectory
//	METHOD TYPE : Constructor
//
//	DESCRIPTION :
//
NCAskForExistingDirectory::NCAskForExistingDirectory( const wpos at,
						      const YCPString & iniDir,
						      const YCPString & headline )
    : NCPopup( at, true )
    , okButton( 0 )
    , cancelButton( 0 )
    , dirName( 0 )
    , dirList( 0 )
    , detailed ( 0 )
{
    createLayout( iniDir, headline );

    dirList->fillDirectoryList( );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCAskForExistingDirectory::~NCAskForExistingDirectory
//	METHOD TYPE : Destructor
//
//	DESCRIPTION :
//
NCAskForExistingDirectory::~NCAskForExistingDirectory()
{

}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCAskForExistingDirectory::createLayout
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCAskForExistingDirectory::createLayout( const YCPString & iniDir,
					      const YCPString & headline )
{

    YWidgetOpt opt;

    // the vertical split is the (only) child of the dialog
    NCSplit * split = new NCSplit( this, opt, YD_VERT );
    addChild( split );

    opt.notifyMode.setValue( false );

    //the headline
    opt.isHeading.setValue( true );

    NCLabel * head = new NCLabel( split, opt, headline );
    split->addChild( head );

    split->addChild( new NCSpacing( split, opt, 0.4, false, true ) );

    opt.isHStretchable.setValue( true );

    NCFrame * frame = new NCFrame( split, opt, YCPString("" ) );
    NCSplit * vSplit = new NCSplit( frame, opt, YD_VERT );

    opt.isEditable.setValue( false );
    opt.notifyMode.setValue( true );
    dirName = new NCComboBox( frame, opt, YCPString("Selected directory:") );
    frame->addChild( dirName );

    dirName->setId( PkgNames::DirName() );

    dirName->itemAdded( YCPString( iniDir ), // set initial value
			0,		 // index
			true );		 // selected
    vSplit->addChild( new NCSpacing( vSplit, opt, 0.6, false, true ) );

    split->addChild( frame );

    // add the checkBox detailed
    NCSplit * hSplit = new NCSplit( split, opt, YD_HORIZ );
    split->addChild( hSplit );
    detailed = new NCCheckBox( hSplit, opt, YCPString( "Details view" ), false );
    detailed->setId( PkgNames::Details() );
    hSplit->addChild( new NCSpacing( hSplit, opt, 0.1, true, false ) );
    hSplit->addChild( detailed );
    
    // add the list of directories
    dirList = new NCFileTable( split, opt, NCFileTable::T_Overview, iniDir );
    dirList->setId( PkgNames::DirList() );
    split->addChild( dirList );

    split->addChild( new NCSpacing( split, opt, 0.4, false, true ) );

    // HBox for the buttons
    NCSplit * hSplit1 = new NCSplit( split, opt, YD_HORIZ );
    split->addChild( hSplit1 ); 
    opt.isHStretchable.setValue( true );
    hSplit1->addChild( new NCSpacing( hSplit1, opt, 0.2, true, false ) );

    // add the OK button
    opt.key_Fxx.setValue( 10 );
    okButton = new NCPushButton( hSplit1, opt, YCPString(PkgNames::OKLabel()) );
    okButton->setId( PkgNames::OkButton() );

    hSplit1->addChild( okButton );

    hSplit1->addChild( new NCSpacing( hSplit1, opt, 0.4, true, false ) );
      
    // add the Cancel button
    opt.key_Fxx.setValue( 9 );
    cancelButton = new NCPushButton( hSplit1, opt, PkgNames::CancelLabel() );
    cancelButton->setId( PkgNames::Cancel() );

    hSplit1->addChild( cancelButton );
    hSplit1->addChild( new NCSpacing( hSplit1, opt, 0.2, true, false ) );  
  
}

///////////////////////////////////////////////////////////////////
//
// NCursesEvent & showDirPopup ()
//
//
NCursesEvent & NCAskForExistingDirectory::showDirPopup( )
{
    postevent = NCursesEvent();

    if ( !dirList )
	return postevent;

    dirList->setKeyboardFocus();

    // event loop
    do {
	popupDialog();
    } while ( postAgain() );

    popdownDialog();


    return postevent;
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCAskForExistingDirectory::niceSize
//	METHOD TYPE : void
//
//	DESCRIPTION :
//


long NCAskForExistingDirectory::nicesize(YUIDimension dim)
{
    return ( dim == YD_HORIZ ? 70 : 20 );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopup::wHandleInput
//	METHOD TYPE : NCursesEvent
//
//	DESCRIPTION :
//
NCursesEvent NCAskForExistingDirectory::wHandleInput( wint_t ch )
{
    if ( ch == 27 ) // ESC
	return NCursesEvent::cancel;

    NCursesEvent retEvent = NCDialog::wHandleInput( ch );

    return retEvent;
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCAskForExistingDirectory::postAgain
//	METHOD TYPE : bool
//
//	DESCRIPTION :
//
bool NCAskForExistingDirectory::postAgain( )
{
    if( !postevent.widget )
	return false;

    postevent.detail = NCursesEvent::NODETAIL;

    YCPValue currentId =  dynamic_cast<YWidget *>(postevent.widget)->id();
    NCMIL << "Current ID: " << currentId->toString() << endl;

    if ( !currentId.isNull() )
    {
	if ( currentId->compare( PkgNames::OkButton () ) == YO_EQUAL )
	{
	    postevent.result = YCPString( dirList->getCurrentDir() );
	    // return false means: close the popup
	    return false;
	}
	else if ( currentId->compare( PkgNames::DirList() ) == YO_EQUAL )
	{
	    unsigned int i = dirName->getListSize();
	    // show the currently selected directory
	    dirName->itemAdded( postevent.result->asString(),
				i,
				true );

	    if ( postevent.reason == YEvent::Activated )
	    {
		bool details = getCheckBoxValue( detailed );
		NCMIL << "Details: " << (details?"true":"false") << endl;
		if ( details )
		{
		    dirList->setTableType( NCFileTable::T_Detailed );
		}
		else
		{
		    dirList->setTableType( NCFileTable::T_Overview );
		}
		// fill directory list
		dirList->fillDirectoryList();
	    }
	}
	else if ( currentId->compare( PkgNames::DirName() ) == YO_EQUAL )
	{
	    dirList->setStartDir( dirName->getValue() );
	    dirList->fillDirectoryList();
	}
	else if ( currentId->compare( PkgNames::Details() ) == YO_EQUAL )
	{
	    bool details = getCheckBoxValue( detailed );
	    NCMIL << "Details: " << (details?"true":"false") << endl;
	    if ( details )
	    {
		dirList->setTableType( NCFileTable::T_Detailed );
	    }
	    else
	    {
		dirList->setTableType( NCFileTable::T_Overview );
	    }
	    // fill directory list
	    dirList->fillDirectoryList(); 
	}
	else
	{
	    postevent.result = YCPNull();
	    return false;
	}
    }

    if (postevent == NCursesEvent::cancel)
    {
	postevent.result = YCPNull();	
	return false;
    }
    
    return true;
}



bool NCAskForExistingDirectory::getCheckBoxValue( NCCheckBox * checkBox )
{
    YCPValue value = YCPNull();

    if ( checkBox )
    {
	value = checkBox->getValue();

	// return whether the option is selected or not
	if ( !value.isNull() )
	{
	    return ( value->asBoolean()->toString() == "true" ? true : false );
	}
    }

    return false;
}