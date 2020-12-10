/* Classic Ladder Project */
/* Copyright (C) 2001-2020 Marc Le Douarain */
/* http://www.sourceforge.net/projects/classicladder */
/* http://sites.google.com/site/classicladder */
/* October 2006 */
/* -------------------- */
/* Symbols - GTK window */
/* -------------------- */
/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h> // i18n
#include <locale.h> // i18n
#include "classicladder.h"
#include "global.h"
#include "edit.h"
#include "classicladder_gtk.h"
#include "vars_names.h"
#include "symbols_gtk.h"
#include "menu_and_toolbar_gtk.h"
#include "preferences.h"
#include "spy_vars_gtk.h"

GtkWidget *SymbolsWindow;
GtkWidget  *SymbolsScrolledWin;
static GtkListStore *ListStore;

// NUM_ARRAY is a hidden column (=number in the symbols array)
enum
{
	NUM_ARRAY,
	VAR_NAME,
	SYMBOL,
	COMMENT,
	NBR_INFOS
};


void DisplaySymbols( void )
{
	GtkTreeIter   iter;
	int ScanSymb;
	
printf("%s here.\n", __FUNCTION__ );
	// put (back) vertical lift at the top position...
	GtkAdjustment * pGtkAdjust = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW(SymbolsScrolledWin) );
	if ( pGtkAdjust )
		gtk_adjustment_set_value( pGtkAdjust, 0.0 );

	gtk_list_store_clear( ListStore );

	for ( ScanSymb=0; ScanSymb<NBR_SYMBOLS; ScanSymb++ )
	{
		// Acquire an iterator
		gtk_list_store_append( ListStore, &iter );

		// fill the element
		gtk_list_store_set( ListStore, &iter,
					NUM_ARRAY, ScanSymb,
                    VAR_NAME, SymbolArray[ ScanSymb ].VarName,
                    SYMBOL, SymbolArray[ ScanSymb ].Symbol,
                    COMMENT, SymbolArray[ ScanSymb ].Comment,
                    -1);
	}
}

/* The callback for the editing of text in our GtkTreeView */
/* data=column number */
// EMC: added a call to DisplaySymbols() so window updates right away
void Callback_TextEdited(GtkCellRendererText *cell, gchar *path_string,
		      gchar *new_text, gpointer data) {

	int OffsetArray = -999;
	StrSymbol * pSymbol;
	GtkTreeModel *treemodel = (GtkTreeModel *)ListStore;
	GtkTreeIter iter;
	char RedispLabelsBoolsVars = FALSE;

	/* Convert the string path to the row that has changed to a GtkIter */
	gtk_tree_model_get_iter (treemodel, &iter, gtk_tree_path_new_from_string (path_string));

	/* Update the GtkTreeModel with the new value */
	gtk_tree_model_get (treemodel, &iter,
						NUM_ARRAY, &OffsetArray,
						-1/*EndOfList*/);
	gtk_list_store_set( ListStore, &iter,
					data, new_text, -1);
//printf( "path=%s, new_text=%s, data_column=%d, offset_array=%d\n",path_string, new_text, (int)data, OffsetArray );
	pSymbol = &SymbolArray[ OffsetArray ];
	switch( (long)data )
	{
		case VAR_NAME:
			if ( new_text[ 0 ]=='\0' )
			{
				// delete symbol line
				pSymbol->VarName[ 0 ] = '\0';
				pSymbol->Symbol[ 0 ] = '\0';
				pSymbol->Comment[ 0 ] = '\0';
				gtk_list_store_set( ListStore, &iter, VAR_NAME, "", SYMBOL, "", COMMENT, "", -1);
				InfosGene->AskConfirmationToQuit = TRUE;
				InfosGene->HasBeenModifiedForExitCode = TRUE;
				RedispLabelsBoolsVars = TRUE;
			}
			else if ( new_text[ 0 ]!='%' )
			{
				ShowMessageBoxError( _("A variable name always start with '%' character !") );
			}
			else
			{
				if (TextParserForAVar( new_text, NULL, NULL, NULL, TRUE/*PartialNames*/ ) )
				{
					strncpy( pSymbol->VarName, new_text, LGT_VAR_NAME-1 );
					pSymbol->VarName[ LGT_VAR_NAME-1 ] = '\0';
					gtk_list_store_set( ListStore, &iter, data, pSymbol->VarName, -1);
					if ( pSymbol->Symbol[0]=='\0' )
						strcpy( pSymbol->Symbol, "***" );
					InfosGene->AskConfirmationToQuit = TRUE;
					InfosGene->HasBeenModifiedForExitCode = TRUE;
					RedispLabelsBoolsVars = TRUE;
				}
				else
				{
					if (ErrorMessageVarParser)
						ShowMessageBoxError( ErrorMessageVarParser );
					else
						ShowMessageBoxError( _("Unknown variable...") );
				}
			}
			break;
		case SYMBOL:
			strncpy( pSymbol->Symbol, new_text, LGT_SYMBOL_STRING-1 );
			pSymbol->Symbol[ LGT_SYMBOL_STRING-1 ] = '\0';
			gtk_list_store_set( ListStore, &iter, data, pSymbol->Symbol, -1);
			InfosGene->AskConfirmationToQuit = TRUE;
			InfosGene->HasBeenModifiedForExitCode = TRUE;
			RedispLabelsBoolsVars = TRUE;
			break; 
		case COMMENT:
			strncpy( pSymbol->Comment, new_text, LGT_SYMBOL_COMMENT-1 );
			pSymbol->Comment[ LGT_SYMBOL_COMMENT-1 ] = '\0';
			gtk_list_store_set( ListStore, &iter, data, pSymbol->Comment, -1);
			InfosGene->AskConfirmationToQuit = TRUE;
			InfosGene->HasBeenModifiedForExitCode = TRUE;
			break;
	}
DisplaySymbols();
	if( RedispLabelsBoolsVars && Preferences.DisplaySymbolsInBoolsVarsWindows )
		UpdateAllLabelsBoolsVars( -1/*ALL Columns*/ );
}


gint SymbolsWindowDeleteEvent( GtkWidget * widget, GdkEvent * event, gpointer data )
{
// Here, we must only toggle the menu check that will call itself the function below to close the window ...
//	gtk_widget_hide( SymbolsWindow );
	SetToggleMenuForSymbolsWindow( FALSE/*OpenedWin*/ );
	// we do not want that the window be destroyed.
	return TRUE;
}

// called per toggle action menu, or at startup (if window saved open or not)...
void OpenSymbolsWindow( GtkAction * ActionOpen, gboolean OpenIt )
{
	if ( ActionOpen!=NULL )
		OpenIt = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION(ActionOpen) );
	if ( OpenIt )
	{
//v0.9.101, crash at startup if menu "View symbols window" checked		DisplaySymbols();
		RestoreWindowPosiPrefs( "Symbols", SymbolsWindow );
		gtk_widget_show (SymbolsWindow);
		gtk_window_present( GTK_WINDOW(SymbolsWindow) );
	}
	else
	{
		RememberWindowPosiPrefs( "Symbols", SymbolsWindow, TRUE/*SaveWindowSize*/ );
		gtk_widget_hide( SymbolsWindow );
	}
}
void RememberSymbolsWindowPrefs( void )
{
//ForGTK3	char WindowIsOpened = GTK_WIDGET_VISIBLE( GTK_WINDOW(SymbolsWindow) );
	char WindowIsOpened = MY_GTK_WIDGET_VISIBLE( SymbolsWindow );
	RememberWindowOpenPrefs( "Symbols", WindowIsOpened );
	if ( WindowIsOpened )
		RememberWindowPosiPrefs( "Symbols", SymbolsWindow, TRUE/*SaveWindowSize*/ );
}

void SymbolsInitGtk()
{
	GtkWidget *vbox;
	GtkWidget *ListView;
	GtkCellRenderer   *renderer;
	long ScanCol;
	char * ColName[] = { "HiddenColNbr!", N_("Variable"), N_("Symbol name"), N_("Comment") };

	SymbolsWindow = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( GTK_WINDOW( SymbolsWindow ), _("Symbols names") );
	gtk_signal_connect( GTK_OBJECT( SymbolsWindow ), "delete_event",
		GTK_SIGNAL_FUNC(SymbolsWindowDeleteEvent), 0 );

	vbox = gtk_vbox_new(FALSE,0);

	/* Create a list-model and the view. */
	ListStore = gtk_list_store_new( NBR_INFOS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
	ListView = gtk_tree_view_new_with_model ( GTK_TREE_MODEL(ListStore) );

	/* Add the columns to the view. */
	for (ScanCol=1; ScanCol<NBR_INFOS; ScanCol++)
	{
		GtkTreeViewColumn *column;
		renderer = gtk_cell_renderer_text_new();
		g_object_set(renderer, "editable", TRUE, NULL);
//TODO? gtk_entry_set_max_length(GTK_ENTRY(  ),9);
		g_signal_connect( G_OBJECT(renderer), "edited", G_CALLBACK(Callback_TextEdited), (gpointer)ScanCol );
		column = gtk_tree_view_column_new_with_attributes( gettext(ColName[ ScanCol ]), renderer, "text", ScanCol, NULL );
		gtk_tree_view_append_column( GTK_TREE_VIEW(ListView), column );
		gtk_tree_view_column_set_resizable( column, TRUE );
		gtk_tree_view_column_set_sort_column_id( column, ScanCol );
	}
//	avail since gtk v2.10...?
//	gtk_tree_view_set_grid_lines( GTK_TREE_VIEW(ListView), GTK_TREE_VIEW_GRID_LINES_BOTH );

	//v0.9.101, now init blank lines here directly, instead of calling DisplaySymbols()
	//at startup if menu "View symbols window" checked... was crashing because undefined NBR_SYMBOLS
	{
		GtkTreeIter   iter;
		int ScanSymb;
//printf("%s: create blank lines...\n", __FUNCTION__ );
		gtk_list_store_clear( ListStore );
		// beware, use NBR_SYMBOLS_DEF and not NBR_SYMBOLS !!! (undefined here)
		for ( ScanSymb=0; ScanSymb<NBR_SYMBOLS_DEF; ScanSymb++ )
			gtk_list_store_append( ListStore, &iter );
	}

	SymbolsScrolledWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (SymbolsScrolledWin),
                                    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	// here we add the view to the scrolled !
	gtk_container_add(GTK_CONTAINER(SymbolsScrolledWin), ListView);
	gtk_box_pack_start(GTK_BOX (vbox), SymbolsScrolledWin, TRUE, TRUE, 0);

//	gtk_widget_set_size_request( SymbolsWindow, 300, 250 ); // minimum size
gtk_window_set_default_size (GTK_WINDOW (SymbolsWindow), -1, 250);

	gtk_widget_show( SymbolsScrolledWin );
	gtk_widget_show( ListView );
	gtk_container_add( GTK_CONTAINER(SymbolsWindow), vbox );
	gtk_window_set_icon_name (GTK_WINDOW( SymbolsWindow ), GTK_STOCK_SELECT_FONT);
	gtk_widget_show( vbox );

//gtk_widget_show (SymbolsWindow);
}

