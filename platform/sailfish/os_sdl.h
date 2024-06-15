/*************************************************************************/
/*  os_sdl.h                                                             */
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

#ifndef OS_SDL_H
#define OS_SDL_H

#include "context_gl_sdl.h"
#include "crash_handler_sdl.h"
#include "drivers/unix/os_unix.h"
#include "os/input.h"
#include "servers/visual_server.h"
//#include "servers/visual/visual_server_wrap_mt.h"
#include "drivers/alsa/audio_driver_alsa.h"
#include "drivers/pulseaudio/audio_driver_pulseaudio.h"
#include "helper_macros.h"
#include "joypad_linux.h"
#include "main/input_default.h"
#include "power_sdl.h"
#include "servers/audio_server.h"
#include "servers/visual/rasterizer.h"
#include <SDL.h>

#if defined(PULSEAUDIO_ENABLED) && !defined(DISABLE_LIBAUDIORESOURCE)
#include <audioresource.h>
#endif

#undef CursorShape
/**
	@author sashikknox <sashikknox@gmail.com>
*/

#define MAX_TOUCHES 20

/** main class of SDL implementation */
class OS_SDL : public OS_Unix {

#if defined(GLES_ENABLED)
	ContextGL_SDL *context_gl;
#endif
	//Rasterizer *rasterizer;
	VisualServer *visual_server;
	VideoMode current_videomode;
	List<String> args;
	SDL_Window *sdl_window;
	// Window xdnd_source_window;
	MainLoop *main_loop;
	SDL_DisplayMode *sdl_display_mode;
	char *xmbstring;
	int xmblen;
	unsigned long last_timestamp;
	unsigned long last_keyrelease_time;
	// ::XIC xic;
	// ::XIM xim;
	// ::XIMStyle xim_style;
	// static void xim_destroy_callback(::XIM im, ::XPointer client_data,
	//		::XPointer call_data);

	Point2i last_mouse_pos;
	bool last_mouse_pos_valid;
	Point2i last_click_pos;
	uint64_t last_click_ms;
	uint32_t last_button_state;
#ifdef TOUCH_ENABLED
	struct {
		int opcode;
		Vector<int> devices;
		// XIEventMask event_mask;
		Map<int, Vector2> state;
		// Map<int, int> index;
		long long index[MAX_TOUCHES];
	} touch;

	int num_touches;
	// int touch_mouse_index;
#endif

	unsigned int get_mouse_button_state(uint32_t button_mask, bool refresh);
	void get_key_modifier_state(Ref<InputEventWithModifiers> state);

	MouseMode mouse_mode;
	Point2i center;

	// void handle_key_event(XKeyEvent *p_event, bool p_echo = false);
	void process_events();
	virtual void delete_main_loop();
	IP_Unix *ip_unix;

	bool force_quit;
	bool minimized;
	bool window_has_focus;
	bool do_mouse_warp;

	// Current cursor shape.
	CursorShape current_cursor;
	SDL_Cursor *cursors[CURSOR_MAX];
	// What's this for?
	SDL_Cursor *null_cursor;

	InputDefault *input;

#ifdef JOYDEV_ENABLED
	JoypadLinux *joypad;
#endif

#ifdef PULSEAUDIO_ENABLED
	AudioDriverPulseAudio driver_pulseaudio;
#ifndef DISABLE_LIBAUDIORESOURCE
	audioresource_t *audio_resource;
#endif
#endif

	// Atom net_wm_icon;

	PowerSDL *power_manager;

	CrashHandler crash_handler;

	int audio_driver_index;
	int video_driver_index;
	unsigned int capture_idle;
	bool maximized;
	//void set_wm_border(bool p_enabled);
	void set_wm_fullscreen(bool p_enabled);

	// typedef xrr_monitor_info *(*xrr_get_monitors_t)(Display *dpy, Window window, Bool get_active, int *nmonitors);
	// typedef void (*xrr_free_monitors_t)(xrr_monitor_info *monitors);
	// xrr_get_monitors_t xrr_get_monitors;
	// xrr_free_monitors_t xrr_free_monitors;
	// void *xrandr_handle;
	// Bool xrandr_ext_ok;

protected:
	virtual int get_video_driver_count() const;
	virtual const char *get_video_driver_name(int p_driver) const;
	virtual int get_current_video_driver() const;

	virtual int get_audio_driver_count() const;
	virtual const char *get_audio_driver_name(int p_driver) const;

	virtual void initialize_core();
	virtual Error initialize(const VideoMode &p_desired, int p_video_driver, int p_audio_driver);
	virtual void finalize();

	virtual void set_main_loop(MainLoop *p_main_loop);

	// void _window_changed(XEvent *event);

	bool is_window_maximize_allowed();

#if SAILFISH_FORCE_LANDSCAPE && SAILFISH_ENABLED
	void fix_touch_position(Vector2 &pos, bool absolute = false);
#endif

public:
#if defined(PULSEAUDIO_ENABLED)
#if !defined(DISABLE_LIBAUDIORESOURCE)
	bool is_audio_resource_acquired;
#endif
	void start_audio_driver();
	void stop_audio_driver();
#endif

	// Really its not a pause ;) just mute all bus streams
	void pause_audio_driver(bool pause = true);

	virtual String get_name();

	virtual void set_cursor_shape(CursorShape p_shape);
	virtual void set_custom_mouse_cursor(const RES &p_cursor, CursorShape p_shape, const Vector2 &p_hotspot);

	void set_mouse_mode(MouseMode p_mode);
	MouseMode get_mouse_mode() const;

	virtual void warp_mouse_position(const Point2 &p_to);
	virtual Point2 get_mouse_position() const;
	virtual int get_mouse_button_state() const;
	virtual void set_window_title(const String &p_title);

	virtual void set_icon(const Ref<Image> &p_icon);

	virtual MainLoop *get_main_loop() const;

	virtual bool can_draw() const;

	virtual void set_clipboard(const String &p_text);
	virtual String get_clipboard() const;

	virtual void release_rendering_thread();
	virtual void make_rendering_thread();
	virtual void swap_buffers();

	virtual String get_config_path() const;
	virtual String get_data_path() const;
	virtual String get_cache_path() const;

	virtual String get_system_dir(SystemDir p_dir) const;

	virtual bool has_touchscreen_ui_hint() const;
	virtual Error shell_open(String p_uri);

	virtual void set_screen_orientation(ScreenOrientation p_orientation);

	virtual void set_video_mode(const VideoMode &p_video_mode, int p_screen = 0);
	virtual VideoMode get_video_mode(int p_screen = 0) const;
	virtual void get_fullscreen_mode_list(List<VideoMode> *p_list, int p_screen = 0) const;

	virtual int get_screen_count() const;
	virtual int get_current_screen() const;
	virtual void set_current_screen(int p_screen);
	virtual Point2 get_screen_position(int p_screen = -1) const;
	virtual Size2 get_screen_size(int p_screen = -1) const;
	virtual int get_screen_dpi(int p_screen = -1) const;
	virtual Point2 get_window_position() const;
	virtual void set_window_position(const Point2 &p_position);
	virtual Size2 get_window_size() const;
	virtual void set_window_size(const Size2 p_size);
	virtual void set_window_fullscreen(bool p_enabled);
	virtual bool is_window_fullscreen() const;
	virtual void set_window_resizable(bool p_enabled);
	virtual bool is_window_resizable() const;
	virtual void set_window_minimized(bool p_enabled);
	virtual bool is_window_minimized() const;
	virtual void set_window_maximized(bool p_enabled);
	virtual bool is_window_maximized() const;
	virtual void request_attention();

	virtual void set_borderless_window(bool p_borderless);
	virtual bool get_borderless_window();
	virtual void set_ime_position(const Point2 &p_pos);

	virtual void move_window_to_foreground();
	virtual void alert(const String &p_alert, const String &p_title = "ALERT!");

	virtual bool is_joy_known(int p_device);
	virtual String get_joy_guid(int p_device) const;

	virtual void _set_use_vsync(bool p_enable);
	//virtual bool is_vsync_enabled() const;

	virtual OS::PowerState get_power_state();
	virtual int get_power_seconds_left();
	virtual int get_power_percent_left();

	virtual bool _check_internal_feature_support(const String &p_feature);

	virtual void force_process_input();
	void run();

	void disable_crash_handler();
	bool is_disable_crash_handler() const;

	virtual Error move_to_trash(const String &p_path);

	virtual LatinKeyboardVariant get_latin_keyboard_variant() const;

	OS_SDL();
};

#endif
