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

   File:       NCPopupDeps.cc

   Author:     Gabriele Strattner <gs@suse.de>
   Maintainer: Michael Andres <ma@suse.de>

/-*/
#include "Y2Log.h"
#include "NCPopupDeps.h"

#include "NCTree.h"
#include "YMenuButton.h"
#include "YDialog.h"
#include "NCSplit.h"
#include "NCSpacing.h"
#include "PkgNames.h"
#include "NCPkgTable.h"
#include "NCMenuButton.h"
#include "NCPushButton.h"
#include "NCPopupInfo.h"
#include "ObjectStatStrategy.h"

using namespace std;

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopupDeps::NCPopupDeps
//	METHOD TYPE : Constructor
//
//	DESCRIPTION :
//
NCPopupDeps::NCPopupDeps( const wpos at, PackageSelector * pkger )
    : NCPopup( at, false )
      , cancelButton( 0 )
      , solveButton( 0 )
      , pkgs( 0 )
      , deps( 0 )
      , depsMenu( 0 )
      , errorLabel1( 0 )
      , errorLabel2( 0 )
      , packager( pkger )
{
    createLayout( YCPString(PkgNames::PackageDeps().str()) );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopupDeps::~NCPopupDeps
//	METHOD TYPE : Destructor
//
//	DESCRIPTION :
//
NCPopupDeps::~NCPopupDeps()
{
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopupDeps::createLayout
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
void NCPopupDeps::createLayout( const YCPString & headline )
{

  YWidgetOpt opt;

  // vertical split is the (only) child of the dialog
  NCSplit * vSplit = new NCSplit( this, opt, YD_VERT );
  addChild( vSplit );

  // opt.vWeight.setValue( 40 );

  opt.notifyMode.setValue( true );

  NCSpacing * sp0 = new NCSpacing( vSplit, opt, 0.8, false, true );
  vSplit->addChild( sp0 );

  // add the headline
  opt.isHeading.setValue( true );
  NCLabel * head = new NCLabel( vSplit, opt, headline );
  vSplit->addChild( head );

  NCSpacing * sp = new NCSpacing( vSplit, opt, 0.4, false, true );
  vSplit->addChild( sp );

  // add the list containing packages with unresolved depemdencies
  pkgs = new NCPkgTable( vSplit, opt );
  pkgs->setPackager( packager );
  // set status strategy
  ObjectStatStrategy * strategy =  new DependencyStatStrategy();
  pkgs->setTableType( NCPkgTable::T_Dependency, strategy );
  vSplit->addChild( pkgs );

  NCSpacing * sp1 = new NCSpacing( vSplit, opt, 0.2, false, true );
  vSplit->addChild( sp1 );

  opt.isHStretchable.setValue( true );
  opt.isHeading.setValue( false );
 
  errorLabel1 = new NCLabel(  vSplit, opt, YCPString("") );
  vSplit->addChild( errorLabel1 );
  errorLabel2 = new NCLabel(  vSplit, opt, YCPString("") );
  vSplit->addChild( errorLabel2 );

  NCSpacing * sp2 = new NCSpacing( vSplit, opt, 0.2, false, true );
  vSplit->addChild( sp2 );

  // add the package list containing the dependencies
  deps = new NCPkgTable( vSplit, opt );
  deps->setPackager( packager );
  // set status strategy
  ObjectStatStrategy * strat = new DependencyStatStrategy();
  deps->setTableType( NCPkgTable::T_DepsPackages, strat );
  vSplit->addChild( deps );
  
  opt.isHStretchable.setValue( false );
  opt.isHeading.setValue( true );
  NCLabel * helplb = new NCLabel( vSplit, opt, YCPString(PkgNames::DepsHelpLine().str()) );
  vSplit->addChild( helplb );
  
  NCSpacing * sp3 = new NCSpacing( vSplit, opt, 0.6, false, true );
  vSplit->addChild( sp3 );
  
  NCSplit * hSplit = new NCSplit( vSplit, opt, YD_HORIZ );
  vSplit->addChild( hSplit );

  opt.isHStretchable.setValue( true );

  // add the solve button
  solveButton = new NCPushButton( hSplit, opt, YCPString(PkgNames::SolveLabel().str()) );
  solveButton->setId( PkgNames::Solve () );
  hSplit->addChild( solveButton );

  NCSpacing * sp4 = new NCSpacing( hSplit, opt, 0.4, true, false );
  hSplit->addChild( sp4 );
  
  // add the cancel button
  cancelButton = new NCPushButton( hSplit, opt, YCPString(PkgNames::CancelIgnore().str()) );
  cancelButton->setId( PkgNames::Cancel () );
  hSplit->addChild( cancelButton );

}

///////////////////////////////////////////////////////////////////
//
//  showDependencies
// 
//
void NCPopupDeps::showDependencies( )
{

    // 	typedef std::list<Result> ResultList;
    PkgDep::ResultList		goodList;
    
    //
    //  struct ErrorResult {
    // 	    RelInfoList unresolvable;
    //	    	
    //  }
    //	typedef std::list<RelInfo> RelInfoList;
    // 	struct RelInfo {
    //      // name of package causing the relation:
    //      PkgName name;
    //  }
    //

    //	typedef std::list<ErrorResult> ErrorResultList;
    PkgDep::ErrorResultList	badList;
       
    NCDBG << "Solving..." << endl ;

    NCPopupInfo info( wpos(10, 10),  YCPString( "" ), YCPString(PkgNames::Solving().str()), false );
    info.setNiceSize( 18, 4 );
    info.popup();
    
    // call the Y2PM::packageManager() to get the "badlist"
    bool success = Y2PM::packageManager().solveInstall( goodList, badList );

    info.popdown();

    if ( !success )
    {
	// evaluate the ErrorResultList
	evaluateErrorResult( badList );

	// fill the list with packages  which have unresolved deps
	fillDepsPackageList( pkgs );
	// show first dependency
	concretelyDependency( pkgs->getCurrentItem() );
	
	showDependencyPopup();    // show the dependencies

	pkgs->setKeyboardFocus();

    }
    
}


void NCPopupDeps::evaluateErrorResult( PkgDep::ErrorResultList errorlist )
{
    int i = 0;
    PkgDep::ErrorResultList::iterator it = errorlist.begin();

    // clear the deps vector !!!
    dependencies.clear();
    
    while ( it != errorlist.end() )
    {
	// save all ErrorResults
	dependencies.push_back( *it );	

	++it;
	i++;
    }
}


bool NCPopupDeps::fillDepsPackageList( NCPkgTable * table )
{
    if ( !table )
	return false;
    
    // create the list of "bad" packages
    vector< PkgDep::ErrorResult>::iterator it = dependencies.begin();
    unsigned int i = 0;
    vector<string> pkgLine;
    pkgLine.reserve(4);
    table->itemsCleared ();
     
    while ( it != dependencies.end() )
    {
	pkgLine.clear();
	  
	PMObjectPtr objPtr = (*it).solvable;
	if ( objPtr )
	{
	    pkgLine.push_back( objPtr->getSelectable()->name() );	// package name
	    string kind = getDependencyKind( (*it) );
	    pkgLine.push_back( kind );
	    // pkgLine.push_back( objPtr->summary() );

	    table->addLine( objPtr->getSelectable()->status(), //  get the package status
			    pkgLine,
			    i,		// the index
			    objPtr );	// the corresponding package pointer
	
	}
	else
	{
	    pkgLine.push_back( (*it).name );
    	    string kind = getDependencyKind( (*it) );
	    pkgLine.push_back( kind );

	    table->addLine( PMSelectable::S_NoInst, // use status not installed
			    pkgLine,
			    i,		// the index
			    PMObjectPtr() );	// no pointer available 
	}
	
	++it;
	i++;
    }

    table->drawList();
    
    return true;
}


string NCPopupDeps::getDependencyKind(  PkgDep::ErrorResult error )
{
    string ret = "";

    // Get the type of the dependency 
    // TO DO: better classification of dependency

    if ( !error.unresolvable.empty() )
    {
	if ( !error.unresolvable.front().is_conflict )
	{
	    ret = PkgNames::RequText().str();
	}
	else
	{
	    ret = PkgNames::UnresText().str();
	}
    }
    else if ( !error.alternatives.empty() )
    {
	ret = PkgNames::NeedsText().str();
    }
    else if ( !error.conflicts_with.empty() )
    {
	ret = PkgNames::ConflictText().str();
	if ( !error.remove_to_solve_conflict.empty() )
	{
	    NCDBG << "REMOVE to solve not empty" << endl;
	}
    }
    else if ( !error.referers.empty() )
    {
	ret = PkgNames::RequByText().str();
    }

    return ret;
}

bool NCPopupDeps::concretelyDependency( int index )
{
    unsigned int size = dependencies.size();
    unsigned int i = 0;
    vector<string> pkgLine;
    pkgLine.reserve(4);
    bool labelSet = false;
    
    deps->itemsCleared();

    if ( index < 0 || (unsigned int)index >= size )
	return false;

    // get the ErrorResult
    PkgDep::ErrorResult error = dependencies[index];
	
    NCDBG << "*** Showing: " << error << endl;	

    // get the dependencies
    
    if ( !error.unresolvable.empty() )
    {
	bool require = false;

	list<PkgDep::RelInfo>::iterator it = error.unresolvable.begin();
	while ( it != error.unresolvable.end() )
	{
	    pkgLine.clear();
	    PMObjectPtr objPtr = (*it).solvable;	// not needed here 

	    if ( !(*it).is_conflict )	// it is a requires dependency
	    {
		pkgLine.push_back( (*it).rel.asString() );
		deps->addLine( PMSelectable::S_NoInst, // use status NOInst
			       pkgLine,
			       i,		// the index
			       PMObjectPtr() );	// null pointer
		require = true;
	    }
	    else
	    {
		pkgLine.push_back( (*it).rel.asString() );
		deps->addLine( PMSelectable::S_NoInst, // use status NOInst
			       pkgLine,
			       i,		// the index
			       PMObjectPtr() );	// null pointer	

	    }
	    
	    ++it;
	    i++;
	}
	if ( require )
	{
	    errorLabel1->setLabel( YCPString(PkgNames::LabelRequire1().str())  );
	    errorLabel2->setLabel( YCPString(PkgNames::LabelRequire2().str()) );
	    labelSet = true;
	}
	else
	{	
	    errorLabel1->setLabel( YCPString(PkgNames::LabelUnresolvable().str()) );
	    errorLabel2->setLabel(  YCPString( "" ) );
	    labelSet = true;
	}
    }
    if ( !error.alternatives.empty() )
    {
	PMObjectPtr lastPtr;

	list<PkgDep::Alternative>::iterator it = error.alternatives.begin();
	while ( it != error.alternatives.end() )
	{
	    pkgLine.clear();
	    PMObjectPtr objPtr = (*it).solvable; 

	    if ( objPtr
		 && ( lastPtr != objPtr ) )
	    {
		pkgLine.push_back( objPtr->getSelectable()->name() );	// package name
		pkgLine.push_back( objPtr->summary() );
	    
		deps->addLine( objPtr->getSelectable()->status(), //  get the package status
			       pkgLine,
			       i,		// the index
			       objPtr );	// the corresponding package
		lastPtr = (*it).solvable;	
	    }
	    ++it;
	    i++;
	}
	if ( !labelSet )
	{
	    errorLabel1->setLabel( YCPString(PkgNames::LabelAlternative().str()) );
	    errorLabel2->setLabel( YCPString( "" ) );
	    labelSet = true;
	}
    }
    if ( !error.conflicts_with.empty() )
    {
	PMObjectPtr lastPtr;
	list<PkgDep::RelInfo>::iterator it = error.conflicts_with.begin();

	while ( it != error.conflicts_with.end() )
	{
	    pkgLine.clear();
	    PMObjectPtr causePtr = error.solvable;
	    PMObjectPtr objPtr = (*it).solvable; 

            // do not show the dependency if the causing package equals this package
	    // or it was the last package
	    if ( objPtr
		 && (causePtr != objPtr)
		 && (lastPtr != objPtr ) )
	    {
		pkgLine.push_back( objPtr->getSelectable()->name() );	// package name
		pkgLine.push_back( objPtr->summary() );
	    
		deps->addLine( objPtr->getSelectable()->status(), //  get the package status
			       pkgLine,
			       i,		// the index
			       objPtr );	// the corresponding package
		lastPtr = (*it).solvable;
	    }
	    else
	    {
		pkgLine.push_back( (*it).name );
		deps->addLine( PMSelectable::S_NoInst, // use status NOInst
			       pkgLine,
			       i,		// the index
			       PMObjectPtr() );	// null pointer
		
	    }
	    
	    ++it;
	    i++;
	}
	if ( !labelSet )
	{
	    errorLabel1->setLabel( YCPString(PkgNames::LabelConflict1().str()) );
	    errorLabel2->setLabel( YCPString(PkgNames::LabelConflict2().str()) );
	    labelSet = true;
	}
    }
    if ( !error.referers.empty()
	 && error.conflicts_with.empty()
	 && error.unresolvable.empty()
	 && error.alternatives.empty() )
    {
	PMObjectPtr lastPtr;
	string lastName = "";
	
	list<PkgDep::RelInfo>::iterator it = error.referers.begin();

	while ( it != error.referers.end() )
	{
	    pkgLine.clear();
	    PMObjectPtr objPtr = (*it).solvable; 

	    if ( objPtr
		 && (lastPtr != objPtr) )
	    {
		pkgLine.push_back( objPtr->getSelectable()->name() );	// package name
		pkgLine.push_back( objPtr->summary() );
	    
		deps->addLine( objPtr->getSelectable()->status(), //  get the package status
			       pkgLine,
			       i,		// the index
			       objPtr );	// the corresponding package

		lastPtr =  (*it).solvable;
		lastName = objPtr->getSelectable()->name();
	    }
	    else if ( (*it).name != lastName )
	    {
		pkgLine.push_back( (*it).name );
		deps->addLine( PMSelectable::S_NoInst, // use status NOInst
			       pkgLine,
			       i,		// the index
			       PMObjectPtr() );	// null pointer

		lastName = (*it).name; 
	    }

	    ++it;
	    i++;
	    
	}
	if ( !labelSet )
	{
	    errorLabel1->setLabel( YCPString(PkgNames::LabelRequBy1().str()) );
	    errorLabel2->setLabel( YCPString(PkgNames::LabelRequBy2().str()) );
	    labelSet = true;
	}
    }

    // show the list
    deps->drawList();
    
    return true;
}


///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopupDeps::showDependencyPopup
//	METHOD TYPE : void
//
//	DESCRIPTION :
//
NCursesEvent NCPopupDeps::showDependencyPopup( )
{
    postevent = NCursesEvent();

    do {
	
	popupDialog();
	
    } while ( postAgain() );

    popdownDialog();

    return postevent;
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopupDeps::niceSize
//	METHOD TYPE : void
//
//	DESCRIPTION :
//

long NCPopupDeps::nicesize(YUIDimension dim)
{
    return ( dim == YD_HORIZ ? NCurses::cols()-15 : NCurses::lines()-5 );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopup::wHandleInput
//	METHOD TYPE : NCursesEvent
//
//	DESCRIPTION :
//
NCursesEvent NCPopupDeps::wHandleInput( int ch )
{
    if ( ch == 27 ) // ESC
	return NCursesEvent::cancel;

    return NCDialog::wHandleInput( ch );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPopupDeps::postAgain
//	METHOD TYPE : bool
//
//	DESCRIPTION :
//
bool NCPopupDeps::postAgain()
{
    if ( ! postevent.widget )
	return false;

    YCPValue currentId =  dynamic_cast<YWidget *>(postevent.widget)->id();

    if ( currentId->compare( PkgNames::Cancel () ) == YO_EQUAL )
    {
	return false;
    }
    else if  ( currentId->compare( PkgNames::Solve () ) == YO_EQUAL )
    {
	// get currently selected package
	PMObjectPtr currentPtr = pkgs->getDataPointer( pkgs->getCurrentItem() );

	// solve the dependencies and create new list of "bad" packages
	PkgDep::ErrorResultList	badList;
	PkgDep::ResultList	goodList;
   
	NCDBG << "Solving..." << endl ;

	// call the Y2PM::packageManager() to get the "badlist"
	bool success = Y2PM::packageManager().solveInstall( goodList, badList );

	if ( !success )
	{
	    // evaluate the ErrorResultList
	    evaluateErrorResult( badList );

	    // fill the list with packages  which have unresolved deps
	    fillDepsPackageList( pkgs );

	    // set current item ( if the package is still there )
	    if ( currentPtr )
	    {
		unsigned int size = pkgs->getNumLines();
		unsigned int index = 0;
		PMObjectPtr pkgPtr;
		while ( index < size )
		{
		    pkgPtr = pkgs->getDataPointer( index );
		    if ( pkgPtr == currentPtr )
		    {
			NCDBG << "Setting current package line: " << index << endl;
			pkgs->setCurrentItem( index );
			break;
		    }
		    index ++;
		}
	    }
	    pkgs->setKeyboardFocus();

	    concretelyDependency( pkgs->getCurrentItem() );
	}
	else	// everything ok
	{
	    pkgs->itemsCleared();
	    deps->itemsCleared();
	    vector<string> line;
	    line.reserve(2);
	    // show a line with text: no conflicts ...
	    line.push_back( PkgNames::NoConflictText().str() );
	    pkgs->addLine( PMSelectable::S_NoInst,
			   line,
			   0,
			   PMObjectPtr() );
	    pkgs->drawList();
	}
    }

    if ( postevent == NCursesEvent::cancel )
    {
	// return false means: close the popup dialog
	return false;
    }
    return true;
}
