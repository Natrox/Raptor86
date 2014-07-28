// Example usage file.

#include "VirtualMachine.h"

#include "SDL.h"
#include "SDL_surface.h"
#include "SDL_render.h"

#undef main

using namespace Raptor::r86;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;

CRITICAL_SECTION g_CSec;

bool g_BufferedRendering = false;

unsigned int g_PixelBuffer[640*480];

// Interrupt 32

void SwitchToBufferedRendering( VirtualMachine* vm )
{
	memset( g_PixelBuffer, 0, sizeof( unsigned int ) * 640 * 480 );

	g_BufferedRendering = true;

	// Confirm the change on the VM side.
	vm->m_Registers->r_GeneralRegisters[0] = 1;
}

// Interrupt 33

void DrawBuffer( VirtualMachine* vm )
{
	SDL_UpdateTexture( texture, NULL, g_PixelBuffer, 640 * sizeof ( unsigned int ) );

	SDL_RenderClear( renderer );
	SDL_RenderCopy( renderer, texture, NULL, NULL );
	SDL_RenderPresent( renderer );
}

void PlotPixelToBuffer( unsigned int px, int x, int y )
{
	g_PixelBuffer[ x + y * 640 ] = px;
}

void PlotPixel( unsigned int px, int x, int y )
{
	if ( g_BufferedRendering )
	{
		PlotPixelToBuffer( px, x*2, y*2 );
		PlotPixelToBuffer( px, x*2+1, y*2 );
		PlotPixelToBuffer( px, x*2, y*2+1 );
		PlotPixelToBuffer( px, x*2+1, y*2+1 );

		return;						 
	}

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

	texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480 );

	VirtualMachine* vm = new VirtualMachine( HeapMemorySizes::HEAP_MEMORY_16M );

	vm->SetInterruptFunction( SwitchToBufferedRendering, 32 );
	vm->SetInterruptFunction( DrawBuffer, 33 );

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
			if ( strcmp( fileName, "\0" ) != 0 ) 
			{
				vm->LoadProgram( fileName );
				g_BufferedRendering = false;
			}
		}

		if ( !g_BufferedRendering )
		{
			EnterCriticalSection( &g_CSec );
			SDL_RenderPresent( renderer );
			LeaveCriticalSection( &g_CSec );
		}

		if ( events.key.keysym.scancode == SDL_SCANCODE_ESCAPE )
		{
			break;
		}

		Sleep( 16 );
	}

	vm->StopVM();
	delete vm;

	SDL_DestroyRenderer( renderer );
	SDL_DestroyTexture( texture );
	SDL_DestroyWindow( window );
}