 /*=============================================================================
 Copyright (C) 2014 Nikolay Nizov <nizovn@gmail.com>

 Portions copyright © 1996-2003 PalmSource, Inc. or its subsidiaries.
 All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 =============================================================================*/

#include <PalmOS.h>

#include "2048Rsc.h"

/***********************************************************************
 *
 *	Entry Points
 *
 ***********************************************************************/


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/
#define creatorID		'2048'
#define versionID		1

#define version20		0x02000000

#define boardX			18
#define boardY			26

#define pieceWidth		30
#define pieceHeight		30

#define pieceCornerDiameter	7

#define pieceFrameWidth		1
#define pieceSpace		pieceFrameWidth		// space between pieces
#define boardFrameMargin	pieceFrameWidth
#define boardFrameWidth		3

#define boardRows		4
#define boardColumns		4

#define numPositions	(boardRows * boardColumns)

#define emptySquareID		0
#define winSquareID		11	

#define TileDelay	150
//Emulator
//#define TileDelay	100000

#define scoreCountCenter	80
#define scoreCountTop		1
#define scoreCountHeight	10
#define scoreCountFontID	boldFont
#define scoreFontWidth		6
#define scoreMaxSize		8

#define tileLargeFontID		ledFont
#define tileSmallFontID		largeBoldFont	
#define tileLarge		10

/***********************************************************************
 *
 *	Internal Structures
 *
 ***********************************************************************/
typedef struct PieceCoordType {
	UInt8	row;	// 0-based
	UInt8	col;	// 0-based
	} PieceCoordType;

typedef struct GameBoardType {
	UInt8	square[numPositions];
	UInt32	score;
	UInt32	best;
	} GameBoardType;


/***********************************************************************
 *
 *	Private global variables
 *
 ***********************************************************************/
static GameBoardType	GameBoard;
static Int16		penX,penY;
static Boolean		boolWon;

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/
static UInt16 StartApplication (void);
static void StopApplication (void);
static Boolean MainFormDoCommand (UInt16 command);
static void MainFormInit (FormPtr frm);
static Boolean MainFormHandleEvent (EventPtr event);
static void AppEventLoop (void);
static Err RomVersionCompatible (UInt32 requiredVersion, UInt16 launchFlags);
static Boolean AppHandleEvent( EventPtr eventP);

static Boolean findPairDown(void);
static UInt8 countTiles(UInt8 TileID); 
static Boolean gameEnded(void);
static void PromptToStartNewGame(void);
static void Victory(void);
static void UpdateScoreDisplay(void);
static UInt8 GetBoard(UInt8 y, UInt8 x);
static UInt8 findTarget(UInt8 y,UInt8 x,UInt8 stop);
static Boolean slideArray(UInt8 y);
static void SetBoard(UInt8 a,UInt8 b,UInt8 c,UInt8 d);
static void rotateBoard(void);
static Boolean moveUp(void);
static Boolean moveRight(void);
static Boolean moveDown(void);
static Boolean moveLeft(void);
static UInt8 addRandom(void);
static void InitGameBoard(void);
static void SaveGameBoard(void);
static void LoadGameBoard(void);
static PieceCoordType PositionToCoord(UInt8 pos);
static void DrawPiece(UInt8 pos);
static void DrawGameBoard(void);
static Boolean HardKeyHandleEvent( EventPtr event );

//***********************************************************************

static Boolean findPairDown(void)
{
	Boolean success = false;
	UInt8 x,y;
	for (x=0;x<boardRows;x++) {
		for (y=0;y<boardRows-1;y++) {
			if (GetBoard(x,y)==GetBoard(x,y+1)) return true;
		}
	}
	return success;
}

static UInt8 countTiles(UInt8 TileID)
{
	UInt8 x;
	UInt8 count=0;
	for (x=0;x<numPositions;x++) {
		if (GameBoard.square[x]==TileID) {
			count++;
		}
	}
	return count;
}

static Boolean gameEnded(void)
{
	Boolean ended = true;
	if (countTiles(emptySquareID)>0) return false;
	if (findPairDown()) return false;
	rotateBoard();
	if (findPairDown()) ended = false;
	rotateBoard();
	rotateBoard();
	rotateBoard();
	return ended;
}

static void PromptToStartNewGame(void)
{
	// Display the new game prompt
	if (FrmAlert(NewGameAlert) == 0)
	{
		InitGameBoard();
		DrawGameBoard();
	}
}

static void Victory(void)
{
	if (FrmAlert(GameWonAlert) == 1)
	{
		InitGameBoard();
		DrawGameBoard();
	}
}

static void UpdateScoreDisplay(void)
{
	Char			text[32];
	Char*			cP;
	UInt16			textLen;
	Int16			x, y;
	FontID			oldFontID;
	RectangleType	r;

	WinPushDrawState();
	
	// Erase the old display, first
	r.topLeft.x = scoreCountCenter - ((scoreFontWidth * scoreMaxSize) >> 1);
	r.topLeft.y = scoreCountTop;
	r.extent.x = scoreFontWidth * scoreMaxSize;
	r.extent.y = scoreCountHeight;
	WinEraseRectangle(&r, 0 /*cornerDiam*/);
	
	oldFontID = FntSetFont(scoreCountFontID);	// change font
	
	cP = text;
	StrIToA(cP, GameBoard.score);
	textLen = StrLen(text);
	
	x = scoreCountCenter - ((textLen * scoreFontWidth)>>1);
	y = scoreCountTop;
	WinDrawChars(text, textLen, x, y);
	
	WinPopDrawState();
}

//GetBoard(row,column);
static UInt8 GetBoard(UInt8 y, UInt8 x) {
	return GameBoard.square[y*boardColumns+x];
}

//findTarget(column,row,stop);
static UInt8 findTarget(UInt8 y,UInt8 x,UInt8 stop)
{
	Int16 t;	//row
	// if the position is already on the first, don't evaluate
	if (x==0) {
		return x;
	}
	for(t=x-1;t>=0;t--) {
		if (GetBoard(t,y)!=emptySquareID) {
			if (GetBoard(t,y)!=GetBoard(x,y)) {
				// merge is not possible, take next position
				return t+1;
			}
			return t;
		} else {
			// we should not slide further, return this one
			if (t==stop) {
				return t;
			}
		}
	}
	// we did not find a
	return x;
}

//slideArray(column);
static Boolean slideArray(UInt8 y)
{
	Boolean success = false;
	UInt8 x,t,stop=0;	//rows

	for (x=0;x<boardRows;x++) {
		if (GetBoard(x,y)!=emptySquareID) {
			t = findTarget(y,x,stop);
			// if target is not original position, then move or merge
			if (t!=x) {
				// if target is not zero, set stop to avoid double merge
				if (GetBoard(t,y)!=emptySquareID) {
					GameBoard.score+=(UInt32)1 << (GetBoard(x,y) + 1);
					if(GameBoard.score > GameBoard.best)GameBoard.best=GameBoard.score;
					stop = t+1;
					GameBoard.square[t*boardColumns+y]=GetBoard(x,y)+1;
				}
				else
					GameBoard.square[t*boardColumns+y]=GetBoard(x,y);
				GameBoard.square[x*boardColumns+y]=emptySquareID;
				success = true;
			}
		}
	}
	return success;
}


static void SetBoard(UInt8 a,UInt8 b,UInt8 c,UInt8 d) {
	GameBoard.square[a*boardRows+b]=GetBoard(c,d);
}

//counter-clockdown
static void rotateBoard(void) {
	UInt8 i,j,n=boardRows;
	UInt8 tmp;
	for (i=0; i<n/2; i++){
		for (j=i; j<n-i-1; j++){
			tmp=GetBoard(i,j);
			SetBoard(i,j,j,n-i-1);
			SetBoard(j,n-i-1,n-i-1,n-j-1);
			SetBoard(n-i-1,n-j-1,n-j-1,i);
			GameBoard.square[(n-j-1)*boardRows+i]=tmp;
		}
	}
}

static Boolean moveUp(void) {
	Boolean success = false;
	UInt8 x;
	for (x=0;x<boardColumns;x++) {
		success |= slideArray(x);
	}
	return success;
}

static Boolean moveRight(void) {
	Boolean success;
	rotateBoard();
	success = moveUp();
	rotateBoard();
	rotateBoard();
	rotateBoard();
	return success;
}

static Boolean moveDown(void) {
	Boolean success;
	rotateBoard();
	rotateBoard();
	success = moveUp();
	rotateBoard();
	rotateBoard();
	return success;
}

static Boolean moveLeft(void) {
	Boolean success;
	rotateBoard();
	rotateBoard();
	rotateBoard();
	success = moveUp();
	rotateBoard();
	return success;
}

static UInt8 addRandom(void) {
	UInt8 i,r,len=0;
	UInt8 n,list[numPositions];

	for ( i=0; i < numPositions ; i++ )
			if (GameBoard.square[i] == emptySquareID) {
				list[len]=i;
				len++;
			}

	if (len>0) {
		r = (UInt8)SysRandom(0) % len;
		i = list[r];
		n = ( (UInt8)SysRandom(0) %10)/9+1;
		GameBoard.square[i]=n;
	}
	return i;
}

/***********************************************************************
 *
 * FUNCTION:     InitGameBoard
 *
 * DESCRIPTION:	Generate a new game.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static void InitGameBoard(void)
{
	UInt8		i;
	GameBoard.score = 0;
	boolWon = false;
	for ( i=0; i < numPositions ; i++ )
		GameBoard.square[i] = emptySquareID;
	addRandom();
	addRandom();
}

/***********************************************************************
 *
 * FUNCTION:     SaveGameBoard
 *
 * DESCRIPTION:	Save game in the application preferences.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static void SaveGameBoard(void)
{
	PrefSetAppPreferences(creatorID,0,versionID,&GameBoard,sizeof(GameBoard),true);
}

/***********************************************************************
 *
 * FUNCTION:     LoadGameBoard
 *
 * DESCRIPTION:	Load saved game from the app preferences.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static void LoadGameBoard(void)
{
	UInt16 GameBoardSize = sizeof(GameBoardType);
	UInt8 pos;
	if (PrefGetAppPreferences(creatorID, 0, &GameBoard, &GameBoardSize, true) == noPreferenceFound){
		GameBoard.best = 0;
		GameBoard.score = 0;
		InitGameBoard();
	}
	boolWon = false;
	for(pos=0;pos<numPositions;pos++)
		if(GameBoard.square[pos] >= winSquareID){
			boolWon = true;
			break;
		}
}

/***********************************************************************
 *
 * FUNCTION:     PositionToCoord
 *
 * DESCRIPTION:	Convert a 0-based position to row and column pair
 *
 * PARAMETERS:	pos		-- piece position (0-based)
 *
 * RETURNED:	row and column
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static PieceCoordType PositionToCoord(UInt8 pos)
{
	PieceCoordType		coord;
	
	ErrFatalDisplayIf( pos >= numPositions, "pos out of bounds" );

	coord.row = pos / boardColumns;
	coord.col = pos % boardColumns;
	
	return( coord );
}

/***********************************************************************
 *
 * FUNCTION:     DrawPiece
 *
 * DESCRIPTION:	Draw the game piece
 *
 * PARAMETERS:	pos		-- piece position (0-based)
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static void DrawPiece(UInt8 pos)
{
	RectangleType	rect;
	PieceCoordType	coord;
	FontID			oldFontID;

		// Compute the piece rectangle
		coord = PositionToCoord(pos);
		//PrvPieceCoordToRect( coord, &rect );
		rect.topLeft.x = boardX + boardFrameMargin + (pieceWidth * coord.col) +
				(coord.col * pieceSpace);
		rect.topLeft.y = boardY + boardFrameMargin + pieceHeight *coord.row +
				(coord.row * pieceSpace);
		rect.extent.x = pieceWidth;
		rect.extent.y = pieceHeight;
		

		//
		// Draw the new piece
		//
		
		if ( GameBoard.square[pos] != emptySquareID )
			{
			FrameBitsType	frameBits;
			Char		text[32];
			UInt16		textLen;
			Int16		textHeight;
			Int16		textWidth;
			Int16		x, y;
			
			// Draw the frame
			frameBits.word = 0;	// initialize the entire structure
			frameBits.bits.cornerDiam = pieceCornerDiameter;
			frameBits.bits.shadowWidth = 0;
			frameBits.bits.width = pieceFrameWidth;
			WinDrawRectangleFrame( frameBits.word, &rect );
			
			// Draw the label
			StrIToA( text, (UInt32)1<<GameBoard.square[pos] );
			textLen = StrLen( text );
			oldFontID = GameBoard.square[pos] >= tileLarge?
				FntSetFont(tileSmallFontID)
				:FntSetFont(tileLargeFontID);
			textHeight = FntLineHeight();
			textWidth = FntCharsWidth( text, textLen );
			x = rect.topLeft.x + ((rect.extent.x - textWidth) / 2);
			y = rect.topLeft.y + ((rect.extent.y - textHeight) /2 );
			WinDrawChars( text, textLen, x, y);
			FntSetFont(oldFontID);
			};

}

/***********************************************************************
 *
 * FUNCTION:     DrawGameBoard
 *
 * DESCRIPTION:	Draw the game board
 *
 * PARAMETERS:	pos		-- piece position (0-based)
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *			vmk	12/17/97	Fixed drawing glitch by initializing the "word"
 *								field of frameBits.
 *
 ***********************************************************************/
static void DrawGameBoard(void)
{
	RectangleType	rect;
	FrameBitsType	frameBits;
	UInt8		pos;
	
	// Draw the board frame
	rect.topLeft.x = boardX;
	rect.topLeft.y = boardY;
	rect.extent.x = (boardColumns * pieceWidth) + (boardFrameMargin * 2) +
			((boardColumns - 1) * pieceSpace);
	rect.extent.y = (boardRows * pieceHeight) + (boardFrameMargin * 2) +
			((boardRows - 1) * pieceSpace);
	WinEraseRectangle( &rect, 0/*cornerDiam*/ );
	
	frameBits.word = 0;	// initialize the entire structure
	frameBits.bits.cornerDiam = 0;
	frameBits.bits.threeD = 0;
	frameBits.bits.shadowWidth = 0;
	frameBits.bits.width = boardFrameWidth;
	WinDrawGrayRectangleFrame ( frameBits.word, &rect );

	// Erase all old pieces
	rect.topLeft.x = boardX + boardFrameMargin;
	rect.topLeft.y = boardY + boardFrameMargin;
	rect.extent.x = boardColumns * (pieceWidth + pieceSpace);
	rect.extent.y = boardRows * (pieceHeight + pieceSpace);
		WinEraseRectangle( &rect, 0/*cornerDiam*/ );
	
	UpdateScoreDisplay();
	// Draw all game pieces
	for ( pos=0; pos < numPositions; pos++ )
		DrawPiece(pos);
}

/***********************************************************************
 *
 * FUNCTION:     StartApplication
 *
 * DESCRIPTION:	Initialize application.
 *
 *						Load board from features, or generate a new board
 *
 * PARAMETERS:   none
 *
 * RETURNED:     0 on success
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static UInt16 StartApplication (void)
{
	// Initialize the random number seed;
	SysRandom( TimGetSeconds() );
	
	LoadGameBoard();

	return( 0 );
}

/***********************************************************************
 *
 * FUNCTION:	StopApplication
 *
 * DESCRIPTION:	Save the current state of the application and close all
 *						forms.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static void StopApplication (void)
{
	SaveGameBoard();
	FrmCloseAllForms ();
}

/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *                             
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	11/15/96	Initial Revision
 *
 ***********************************************************************/
static Err RomVersionCompatible (UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
		{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
			{
			FrmAlert (RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < 0x02000000)
				{
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
				}
			}
		
		return (sysErrRomIncompatible);
		}

	return (0);
}

/***********************************************************************
 *
 * FUNCTION:    MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:  command  - menu item id
 *
 * RETURNED:    true if the command was handled
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static Boolean MainFormDoCommand (UInt16 command)
{
	Boolean		handled = false;

	MenuEraseStatus (0);

	switch (command)
		{
		case MainOptionsAboutCmd:
			FrmAlert(AboutAlert);
			handled = true;
			break;

		case MainOptionsInstructions:
			FrmHelp (InstructionsStr);
			break;
					
		case MainOptionsBestScore:
			{
			Char text[32];
			Char* cP;
			cP = text;
			StrIToA(cP, GameBoard.best);
			FrmCustomAlert(BestScoreAlert,cP,"","");
			handled = true;
			break;
			};
		}
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:    MainFormInit
 *
 * DESCRIPTION: This routine initializes the "Main View"
 *
 * PARAMETERS:  frm  - a pointer to the MainForm form
 *
 * RETURNED:    nothing.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Version
 *
 ***********************************************************************/
static void MainFormInit (FormPtr frm)
{
}

static void GameCore(Boolean success)
{
	if (success) {
			DrawGameBoard();
			SysTaskDelay( SysTicksPerSecond() * TileDelay / 1000);
			DrawPiece(addRandom());
			if (gameEnded()) {
				PromptToStartNewGame();
			};
			if ((!boolWon)&&(countTiles(winSquareID)>0)){
				boolWon = true;
				Victory();
			};
	};
}
/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the "Main View"
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			roger	8/7/95	Initial Revision
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent (EventPtr event)
{
	FormPtr frm;
	Boolean handled = false;
	Boolean success = false;

	if (event->eType == ctlSelectEvent)
		{
		switch (event->data.ctlSelect.controlID)
			{
			case MainFormNewGameButton:
				InitGameBoard();
				DrawGameBoard();
				handled = true;
				break;
				
			default:
				break;
			}
		}
	
	else if (event->eType == penDownEvent)
		{
			penX=event->screenX;
			penY=event->screenY;
		}

	else if (event->eType == penUpEvent)
		{
			Int16 offsetX=event->screenX-penX;
			Int16 offsetY=event->screenY-penY;
			Boolean signX = offsetX > 0;
			Boolean signY = offsetY > 0;
			Int16 ExtentX = (boardColumns * pieceWidth) +
				(boardFrameMargin * 2) +
				((boardColumns - 1) * pieceSpace);
			Int16 ExtentY = (boardRows * pieceHeight) +
				(boardFrameMargin * 2) +
				((boardRows - 1) * pieceSpace);
			offsetX=signX?offsetX:-offsetX;
			offsetY=signY?offsetY:-offsetY;
			if (((offsetX > pieceWidth) ||
			   (offsetY > pieceHeight))
				&& (penX < (boardX+ExtentX))
				&& (penX > boardX)
				&& (penY > boardY)
				&& (penY < (boardY+ExtentY)))
			{
				if (offsetX > offsetY)
					if (signX)
						success = moveRight();
					else
						success = moveLeft();
				else
					if (!signY)
						success = moveUp();
					else
						success = moveDown();
				handled = true;
			}

		}
				
	else if (event->eType == menuEvent)
		{
		return MainFormDoCommand (event->data.menu.itemID);
		}


	else if (event->eType == frmUpdateEvent)
		{
		FrmDrawForm (FrmGetActiveForm());
		DrawGameBoard();
		handled = true;
		}
		
	else if (event->eType == frmOpenEvent)
		{
		frm = FrmGetActiveForm();
		MainFormInit (frm);
		FrmDrawForm (frm);
		DrawGameBoard();
		handled = true;
		}
		
	else if (event->eType == frmCloseEvent)
		{
		}
	GameCore(success);
	return (handled);
}

static Boolean HardKeyHandleEvent( EventPtr event )
{
	Boolean handled = false;
	Boolean success = false;
	if (event->eType == keyDownEvent)
		{
		if (TxtCharIsHardKey(event->data.keyDown.modifiers, event->data.keyDown.chr))
			{
				switch(event->data.keyDown.chr) {
					case vchrHard1:
					case vchrHard2:
						success = moveLeft();
						handled = true;
						break;
					case vchrHard3:
					case vchrHard4:
						success = moveRight();
						handled=true;
						break;
					default: success = false;
				};
			}
		else if (EvtKeydownIsVirtual(event))
			{
			switch(event->data.keyDown.chr) {
					case vchrPageUp:
					success=moveUp();
					handled=true;
					break;
					case vchrPageDown:
					success=moveDown();
					handled=true;
					break;
				default: success = false;
				};
			}	
	};
	GameCore(success);
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean AppHandleEvent( EventPtr eventP)
{
	UInt16 formId;
	FormPtr frmP;

	if (eventP->eType == frmLoadEvent)
		{
		// Load the form resource.
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
			{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

			default:
				ErrNonFatalDisplay("Invalid Form Load Event");
				break;

			}
		return true;
		}
	
	return false;
}

/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;

	do {
		EvtGetEvent(&event, evtWaitForever);
		if (! HardKeyHandleEvent(&event))
			if (! SysHandleEvent(&event))
				if (! MenuHandleEvent(0, &event, &error))
					if (! AppHandleEvent(&event))
						FrmDispatchEvent(&event);

		// Check the heaps after each event
		#if EMULATION_LEVEL != EMULATION_NONE
			MemHeapCheck(0);
			MemHeapCheck(1);
		#endif

	} while (event.eType != appStopEvent);
}

/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the Puzzle 
 *              application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    0
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			vmk	8/19/95	Initial Revision
 *
 ***********************************************************************/
UInt32 PilotMain (UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	UInt16 error;

	error = RomVersionCompatible (version20, launchFlags);
	if (error) return (error);

	if ( cmd == sysAppLaunchCmdNormalLaunch )
		{
		error = StartApplication ();
		if (error) return (error);

		FrmGotoForm (MainForm);

		AppEventLoop ();
		StopApplication ();
		}

	return (0);
}
