// Example usage file.

#include "VirtualMachine.h"

#include "SDL.h"
#include "SDL_surface.h"

#undef main

using namespace Raptor::r86;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screen = NULL;

CRITICAL_SECTION g_CSec;

void PlotPixel( unsigned int px, int x, int y )
{
	EnterCriticalSection( &g_CSec );

	static SDL_Rect dstRect;

	dstRect.x = x * 2;
	dstRect.y = y * 2;

	dstRect.w = 2;
	dstRect.h = 2;

	SDL_SetRenderDrawColor( renderer, ( px & 0xff0000 ) >> 16, ( px & 0xff00 ) >> 8, px & 0xff, 0xff );
	SDL_RenderFillRect( renderer, &dstRect );

	LeaveCriticalSection( &g_CSec );
}

void ClearScreen( unsigned int px )
{
	EnterCriticalSection( &g_CSec );

	static SDL_Rect dstRect;

	dstRect.x = 0;
	dstRect.y = 0;

	dstRect.w = 640;
	dstRect.h = 480;

	SDL_SetRenderDrawColor( renderer, ( px & 0xff0000 ) >> 16, ( px & 0xff00 ) >> 8, px & 0xff, 0xff );
	SDL_RenderFillRect( renderer, &dstRect );

	LeaveCriticalSection( &g_CSec );
}

bool CheckKey( int key )
{
	return GetAsyncKeyState( key ) != 0;
}

int main( void )
{
	InitializeCriticalSection( &g_CSec );

	SDL_Init( SDL_INIT_EVERYTHING );

	window = SDL_CreateWindow( "Raptor86 VM Video (F1 to load program)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320 * 2, 240 * 2, SDL_WINDOW_SHOWN );
	renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );

	VirtualMachine* vm = new VirtualMachine( HeapMemorySizes::HEAP_MEMORY_16M );

	vm->StartSuspendedVM();
	vm->SetScreenClearFunction( ClearScreen );
	vm->SetPlotPixelFunction( PlotPixel );
	vm->SetCheckKeyFunction( CheckKey );
	vm->ResumeVM();

	while ( 1 )
	{
		static SDL_Event events;

		SDL_PollEvent( &events );

		if ( GetAsyncKeyState( VK_F1 ) )
		{
			OPENFILENAMEA file;

			memset( &file, 0, sizeof( OPENFILENAMEA ) );

			char fileName[4096];

			file.lStructSize = sizeof( file );
			file.hwndOwner = 0;
			file.lpstrFile = fileName;
			file.lpstrFile[0] = '\0';
			file.nMaxFile = sizeof( fileName );
			file.lpstrFilter = "Raptor86 Program (*.r86)\0*.R86\0Raptor86 Program (*.rvm)\0*.RVM\0";
			file.nFilterIndex = 1;
			file.lpstrFileTitle = 0;
			file.nMaxFileTitle = 0;
			file.lpstrInitialDir = 0;
			file.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;

			GetOpenFileNameA( &file );

			ClearScreen( 0 );
			if ( strcmp( fileName, "\0" ) != 0 ) vm->LoadProgram( fileName );
		}

		EnterCriticalSection( &g_CSec );
		SDL_RenderPresent( renderer );
		LeaveCriticalSection( &g_CSec );

		if ( events.key.keysym.scancode == SDL_SCANCODE_ESCAPE )
		{
			break;
		}

		Sleep( 16 );
	}

	vm->StopVM();
	delete vm;

	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
}