/*************************************************************************/
/*  context_gl_sdl.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "context_gl_sdl.h"

#ifdef SDL_ENABLED
#if defined(GLES2_ENABLED)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// #include <GLES2/gl2.h>
#include "helper_macros.h"
#include <EGL/egl.h>
#include <SDL_opengles2.h>
#include <SDL_video.h>
#ifdef SAILFISH_ENABLED
#include <SDL_syswm.h>
#include <wayland-client-protocol.h>
#endif
#include <string>
#include <vector>

struct ContextGL_SDL_Private {
	SDL_GLContext gl_context;
	int display_index;
	OS::ScreenOrientation allowed_orientation_enum;
	std::string allowed_orientation_str;
	int wl_allowed_orientation;
};

void ContextGL_SDL::release_current() {
	SDL_GL_MakeCurrent(sdl_window, NULL);
}

void ContextGL_SDL::make_current() {
	SDL_GL_MakeCurrent(sdl_window, p->gl_context);
}

void ContextGL_SDL::swap_buffers() {
	SDL_GL_SwapWindow(sdl_window);
}

Error ContextGL_SDL::initialize() {
	mprint_verbose("Begin SDL2 initialization\n");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	if (opengl_3_context == true) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	} 
	 else {
	// // Try OpenGL ES 2.0
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	}
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_DisplayMode dm;
	OS::get_singleton()->print("Get display mode\n");
	SDL_GetCurrentDisplayMode(0, &dm);
	OS::get_singleton()->print("Resolution is: %ix%i\n", dm.w, dm.h);
	OS::get_singleton()->print("Try create SDL_Window\n");
	width = dm.w;
	height = dm.h;

	// dm.orientation;

	// SDL_GetDisplayMode()
	String app_name = OS::get_singleton()->get_name();
	if (app_name.empty())
		app_name = "Godot";
	sdl_window = SDL_CreateWindow(app_name.utf8().ptr(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, dm.w, dm.h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN); //| SDL_WINDOW_FULLSCREEN
	// FIXME test QtWayland flags
	// SDL_SetHintWithPriority(SDL_HINT_QTWAYLAND_WINDOW_FLAGS, "StayOnTop", SDL_HINT_OVERRIDE);

	if (!sdl_window) {
		OS::get_singleton()->print("SDL_Error \"%s\"", SDL_GetError());
		return FAILED;
	}
	else {
		struct SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		if (SDL_GetWindowWMInfo(sdl_window, &wmInfo)) {
			wl_surface_set_buffer_transform(wmInfo.info.wl.surface, p->wl_allowed_orientation);
		}
	}
	ERR_FAIL_COND_V(!sdl_window, ERR_UNCONFIGURED);

	p->display_index = SDL_GetWindowDisplayIndex(sdl_window);
	OS::get_singleton()->print("DisplayIndex is %i \n", p->display_index);

	OS::get_singleton()->print("\nSDL_RENDER_DRIVER available:\n");
	for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
		SDL_RendererInfo info;
		SDL_GetRenderDriverInfo(i, &info);
		OS::get_singleton()->print("[%i] %s\n", i, info.name);
	}

	mprint_verbose("Create GL context.\n");
	p->gl_context = SDL_GL_CreateContext(sdl_window); //SDL_GL_GetCurrentContext();
	if (p->gl_context == NULL) {
		// ERR_EXPLAIN("Could not obtain an OpenGL ES 2.0 context!");
		OS::get_singleton()->print("ERROR: Could not obtain an OpenGL ES 3.0 context!");
		ERR_FAIL_COND_V(p->gl_context == NULL, ERR_UNCONFIGURED);
		return FAILED;
	}
	return OK;
}

int ContextGL_SDL::get_window_width() {
	// int w;
	// SDL_GetWindowSize(sdl_window, &w, NULL);

	return width;
}

int ContextGL_SDL::get_window_height() {
	// int h;
	// SDL_GetWindowSize(sdl_window, NULL, &h);

	return height;
}

void ContextGL_SDL::set_use_vsync(bool p_use) {
	if (p_use) {
		if (SDL_GL_SetSwapInterval(1) < 0) printf("Warning: Unable to enable vsync! SDL Error: %s\n", SDL_GetError());
	} else {
		if (SDL_GL_SetSwapInterval(0) < 0) printf("Warning: Unable to disable vsync! SDL Error: %s\n", SDL_GetError());
	}

	use_vsync = p_use;
}

bool ContextGL_SDL::is_using_vsync() const {
	return use_vsync;
}

SDL_Window *ContextGL_SDL::get_window_pointer() {
	return sdl_window;
}

void ContextGL_SDL::set_ext_surface_orientation(int sdl_orientation) {
	OS::ScreenOrientation screen_orientation = OS::get_singleton()->get_screen_orientation();
#ifdef SAILFISH_ENABLED
	int wl_orientation = p->wl_allowed_orientation;
	/*case SDL_ORIENTATION_LANDSCAPE: WL_OUTPUT_TRANSFORM_270
	case SDL_ORIENTATION_LANDSCAPE_FLIPPED: WL_OUTPUT_TRANSFORM_90
	case SDL_ORIENTATION_PORTRAIT: WL_OUTPUT_TRANSFORM_NORMAL
	case SDL_ORIENTATION_PORTRAIT_FLIPPED: WL_OUTPUT_TRANSFORM_180
	case SDL_ORIENTATION_UNKNOWN: WL_OUTPUT_TRANSFORM_90*/
#endif
	mprint_verbose2("OS current screen orientation is \"%i\"\n", screen_orientation);
	switch (p->allowed_orientation_enum) {
		case OS::SCREEN_SENSOR_LANDSCAPE: {
			switch (sdl_orientation) {
				case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
					// qt_extended_surface_set_content_orientation(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_LANDSCAPEORIENTATION);
					p->allowed_orientation_str = "landscape";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_270;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_Screen_orientation OS::SCREEN_LANDSCAPE\n");
					screen_orientation = (OS::SCREEN_LANDSCAPE);
					break;
				case SDL_ORIENTATION_LANDSCAPE:
					// qt_extended_surface_set_content_orientation(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_INVERTEDLANDSCAPEORIENTATION);
					p->allowed_orientation_str = "inverted-landscape";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_90;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_Screen_orientation OS::SCREEN_REVERSE_LANDSCAPE\n");
					screen_orientation = (OS::SCREEN_REVERSE_LANDSCAPE);
					break;
			}
		} break;
		case OS::SCREEN_SENSOR_PORTRAIT: {
			switch (sdl_orientation) {
				case SDL_ORIENTATION_PORTRAIT:
					// qt_extended_surface_set_content_orientation(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_PORTRAITORIENTATION);
					p->allowed_orientation_str = "portrait";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_NORMAL;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_screen_orientation OS::SCREEN_PORTRAIT\n");
					screen_orientation = (OS::SCREEN_PORTRAIT);
					break;
				case SDL_ORIENTATION_PORTRAIT_FLIPPED:
					// qt_extended_surface_set_content_orientation(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_INVERTEDPORTRAITORIENTATION);
					p->allowed_orientation_str = "inverted-portrait";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_180;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_screen_orientation OS::SCREEN_REVERSE_PORTRAIT\n");
					screen_orientation = (OS::SCREEN_REVERSE_PORTRAIT);
					break;
			}
		} break;
		case OS::SCREEN_SENSOR:
			switch (sdl_orientation) {
				case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
					// qt_extended_surface_set_content_orientation(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_LANDSCAPEORIENTATION);
					p->allowed_orientation_str = "landscape";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_270;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_screen_orientation OS::SCREEN_LANDSCAPE\n");
					screen_orientation = (OS::SCREEN_LANDSCAPE);
					break;
				case SDL_ORIENTATION_LANDSCAPE:
					// qt_extended_surface_set_content_orientation(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_INVERTEDLANDSCAPEORIENTATION);
					p->allowed_orientation_str = "inverted-landscape";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_90;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_screen_orientation OS::SCREEN_REVERSE_LANDSCAPE\n");
					screen_orientation = (OS::SCREEN_REVERSE_LANDSCAPE);
					break;
				case SDL_ORIENTATION_PORTRAIT:
					// qt_extended_surface_set_content_orientation0(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_PORTRAITORIENTATION);
					p->allowed_orientation_str = "portrait";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_NORMAL;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_screen_orientation OS::SCREEN_PORTRAIT\n");
					screen_orientation = (OS::SCREEN_PORTRAIT);
					break;
				case SDL_ORIENTATION_PORTRAIT_FLIPPED:
					// qt_extended_surface_set_content_orientation(p->qt_ext_surface, QT_EXTENDED_SURFACE_ORIENTATION_INVERTEDPORTRAITORIENTATION);
					p->allowed_orientation_str = "inverted-portrait";
					#ifdef SAILFISH_ENABLED
					wl_orientation = WL_OUTPUT_TRANSFORM_180;
					#endif
					if (OS::get_singleton()->is_stdout_verbose())
						OS::get_singleton()->print("set_screen_orientation OS::SCREEN_REVERSE_PORTRAIT\n");
					screen_orientation = (OS::SCREEN_REVERSE_PORTRAIT);
					break;
			}
			break;
		default:
			// No need other orietations hadle, bacuse other orientations are static, not dynamic
			break;
	}
#ifdef SAILFISH_ENABLED
	struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(sdl_window, &wmInfo)) {
#endif
		mprint_verbose2("SDL_SetHint(%s, %s)\n", SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION, p->allowed_orientation_str.c_str());
        if (SDL_SetHintWithPriority(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION, p->allowed_orientation_str.c_str(), SDL_HINT_OVERRIDE) == SDL_FALSE) {
			mprint_verbose("WARGNING: Cant set hint SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION for orinetation events");
		}
#ifdef SAILFISH_ENABLED
    } else if(p->wl_allowed_orientation != wl_orientation)  {
		p->wl_allowed_orientation = wl_orientation;
		wl_surface_set_buffer_transform(wmInfo.info.wl.surface, wl_orientation);
	}
#endif
	OS::get_singleton()->set_screen_orientation(screen_orientation);
}

void ContextGL_SDL::set_screen_orientation(OS::ScreenOrientation p_orientation) {
#if SAILFISH_FORCE_LANDSCAPE
	switch (p_orientation) {
		case OS::SCREEN_LANDSCAPE:
			p->allowed_orientation_str = "landscape";
			p->wl_allowed_orientation = WL_OUTPUT_TRANSFORM_90;
			break;
		case OS::SCREEN_PORTRAIT:
			p->allowed_orientation_str = "portrait";
			p->wl_allowed_orientation = WL_OUTPUT_TRANSFORM_NORMAL;
			break;
		case OS::SCREEN_REVERSE_LANDSCAPE:
			p->allowed_orientation_str = "inverted-landscape";
			p->wl_allowed_orientation = WL_OUTPUT_TRANSFORM_270;
			break;
		case OS::SCREEN_REVERSE_PORTRAIT:
			p->allowed_orientation_str = "inverted-portrait";
			p->wl_allowed_orientation = WL_OUTPUT_TRANSFORM_180;
			break;
		case OS::SCREEN_SENSOR_LANDSCAPE:
			p->allowed_orientation_str = "landscape";
			p->wl_allowed_orientation = WL_OUTPUT_TRANSFORM_90;
			break;
		case OS::SCREEN_SENSOR_PORTRAIT:
			p->allowed_orientation_str = "portrait";
			p->wl_allowed_orientation = WL_OUTPUT_TRANSFORM_NORMAL;
			break;
		case OS::SCREEN_SENSOR:
			p->allowed_orientation_str = "primary";
			p->wl_allowed_orientation = WL_OUTPUT_TRANSFORM_NORMAL;
			break;
	}

	if (OS::get_singleton()->is_stdout_verbose())
		OS::get_singleton()->print("Set allowed orientations to \"%s\"\n", p->allowed_orientation_str.c_str());

	/*
	"primary" (default)
	"landscape"          top of device left
	"inverted-landscape" top of device right
	"portrait"           top of device up
	"inverted-portrait"  top of device down
	*/
#ifndef SAILFISH_ENABLED
	if (SDL_SetHintWithPriority(SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION, p->allowed_orientation_str.c_str(), SDL_HINT_OVERRIDE) == SDL_FALSE) {
		mprint_verbose("WARGNING: Cant set hint SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION for orinetation events");
		// OS::get_singleton()->print
	} else
		mprint_verbose2("SDL_HINT_QTWAYLAND_CONTENT_ORIENTATION sets to %s\n", p->allowed_orientation_str.c_str());
#else
	if(sdl_window)
	{
		struct SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		if (SDL_GetWindowWMInfo(sdl_window, &wmInfo)) {
			wl_surface_set_buffer_transform(wmInfo.info.wl.surface, p->wl_allowed_orientation);
		}
	}
#endif

#endif
	p->allowed_orientation_enum = p_orientation;
}

ContextGL_SDL::ContextGL_SDL(::SDL_DisplayMode *p_sdl_display_mode, const OS::VideoMode &p_default_video_mode, bool p_opengl_3_context) {
	default_video_mode = p_default_video_mode;
	sdl_display_mode = p_sdl_display_mode;
	opengl_3_context = p_opengl_3_context;
	p = memnew(ContextGL_SDL_Private);
	p->gl_context = NULL;
	use_vsync = false;
	sdl_window = nullptr;
}

ContextGL_SDL::~ContextGL_SDL() {
	SDL_GL_DeleteContext(p->gl_context);
	SDL_DestroyWindow(sdl_window);
	memdelete(p);
}

#endif
#endif
