#include "tetris.h"

#include <android/log.h>

#include <jni.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <android/input.h>
#include <android/native_activity.h>

#define APP_CMD_INPUT_QUEUE_DESTROYED  1
#define APP_CMD_WINDOW_DESTROYED       2
#define APP_CMD_WINDOW_CREATED         3
#define APP_CMD_CONFIG_CHANGED         4
#define APP_CMD_LOW_MEMORY             5
#define APP_CMD_WINDOW_REDRAW_NEEDED   6
#define APP_CMD_WINDOW_RESIZED         7
#define APP_CMD_PAUSE                  8
#define APP_CMD_RESUME                 9
#define APP_CMD_START                 10
#define APP_CMD_STOP                  11
#define APP_CMD_DESTROY               12
#define APP_CMD_GAINED_FOCUS          13
#define APP_CMD_LOST_FOCUS            14
#define APP_CMD_CONTENT_RECT_CHANGED  15

struct {
	int cmd_queue[0x200];
	int cmd_read_travel;
	int cmd_write_travel;

	ANativeActivity *activity;
	ANativeWindow *window;
	AInputQueue *input_queue;
	pthread_t thread;
	ARect pending_rect;
	bool running;
	bool egl_available;

	float grid_colors[GRID_COLOR_FLOATS];
	GL_Context egl;
	Tetris tetris;
	int frame_counter;
} static app_global = {0};

void set_window_egl_format(void *window, int format)
{
	ANativeWindow_setBuffersGeometry((ANativeWindow*)window, 0, 0, format);
}

void handle_app_command(int cmd)
{
	switch (cmd) {
		case APP_CMD_INPUT_QUEUE_DESTROYED:
		{
			app_global.input_queue = NULL;
			break;
		}
		case APP_CMD_WINDOW_DESTROYED:
		{
			app_global.window = NULL;
			app_global.egl_available = false;
			app_global.egl.quit();
			break;
		}
		case APP_CMD_WINDOW_CREATED:
		{
			app_global.egl.init(app_global.window, app_global.grid_colors);
			app_global.egl_available = true;
			break;
		}
		case APP_CMD_WINDOW_REDRAW_NEEDED:
		{
			break;
		}
		case APP_CMD_WINDOW_RESIZED:
		{
			break;
		}
		case APP_CMD_PAUSE:
		{
			break;
		}
		case APP_CMD_RESUME:
		{
			break;
		}
		case APP_CMD_START:
		{
			break;
		}
		case APP_CMD_STOP:
		{
			break;
		}
		case APP_CMD_DESTROY:
		{
			app_global.egl_available = false;
			app_global.egl.quit();
			app_global.running = false;
			break;
		}
		case APP_CMD_GAINED_FOCUS:
		{
			break;
		}
		case APP_CMD_LOST_FOCUS:
		{
			break;
		}
		case APP_CMD_CONTENT_RECT_CHANGED:
		{
			ARect rect = app_global.pending_rect;
			break;
		}
	}
}

int locate_input(float x, float y)
{
    int idx = 0;
    if (y >= 0.82) {
        idx = 5;
    }
	else if (y < 0.20) {
		idx = 6;
	}
    else {
        if (x >= 0.80)
            idx = 2;
        else if (x >= 0.56)
            idx = 1;
        else if (x <= 0.20)
            idx = 3;
        else if (x <= 0.44)
            idx = 4;
    }
    return idx;
}

bool handle_motion_input(AInputEvent *event)
{
	int input = AMotionEvent_getAction(event);
	int index = (input & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
		>> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    int action = input & AMOTION_EVENT_ACTION_MASK;
	bool down = false;

	if (action == AMOTION_EVENT_ACTION_DOWN ||
		action == AMOTION_EVENT_ACTION_POINTER_DOWN
	) {
		down = true;
	}
	else if (action == AMOTION_EVENT_ACTION_UP ||
		action == AMOTION_EVENT_ACTION_CANCEL ||
		action == AMOTION_EVENT_ACTION_OUTSIDE ||
		action == AMOTION_EVENT_ACTION_POINTER_UP
	) {
		down = false;
	}
	else {
		return true;
	}

	float x = AMotionEvent_getX(event, index);
	float y = AMotionEvent_getY(event, index);
	int id = AMotionEvent_getPointerId(event, index);

	if (id < 0 || id >= MAX_INPUTS)
		return true;

	int section = 0;
	if (down) {
		float width = (float)app_global.egl.width;
		float height = (float)app_global.egl.height;
		section = locate_input(x / width, y / height);
	}

	app_global.tetris.input_sections[id] = section;
	app_global.tetris.input_timers[id] = 0;

	return true;
}

void *app_main(void *data)
{
	app_global.running = true;

	while (true) {
		int cmd = app_global.cmd_queue[app_global.cmd_read_travel & 0x1ff];
		if (cmd) {
			app_global.cmd_queue[app_global.cmd_read_travel & 0x1ff] = 0;
			app_global.cmd_read_travel++;
			handle_app_command(cmd);
		}

		AInputEvent *event = NULL;
		while (app_global.input_queue && AInputQueue_getEvent(app_global.input_queue, &event) >= 0) {
			if (AInputQueue_preDispatchEvent(app_global.input_queue, event))
				continue;

			bool handled = false;
			int type = AInputEvent_getType(event);
			if (type == AINPUT_EVENT_TYPE_MOTION)
				handled = handle_motion_input(event);

			AInputQueue_finishEvent(app_global.input_queue, event, handled);
		}

		if (!app_global.running)
			break;

		if (!app_global.egl_available) {
			usleep(10000);
			continue;
		}

		Tetris *game = &app_global.tetris;
		game->update();
		game->refresh_colors(app_global.grid_colors);
		
		app_global.egl.draw(
			app_global.grid_colors,
			app_global.frame_counter++,
			game->pause_position,
			game->score
		);
	}

	return NULL;
}

void push_app_command(int cmd)
{
	app_global.cmd_queue[app_global.cmd_write_travel++ & 0x1ff] = cmd;
}

static void onInputQueueCreated(ANativeActivity *activity, AInputQueue *queue) {
	app_global.input_queue = queue;
}
static void onInputQueueDestroyed(ANativeActivity *activity, AInputQueue *queue) {
	//app_global.input_queue = NULL;
	push_app_command(APP_CMD_INPUT_QUEUE_DESTROYED);
}
static void onNativeWindowCreated(ANativeActivity *activity, ANativeWindow *window) {
	app_global.window = window;
	push_app_command(APP_CMD_WINDOW_CREATED);
}
static void onNativeWindowDestroyed(ANativeActivity *activity, ANativeWindow *window) {
	//app_global.window = NULL;
	push_app_command(APP_CMD_WINDOW_DESTROYED);
}

static void onConfigurationChanged(ANativeActivity *activity) {
	push_app_command(APP_CMD_CONFIG_CHANGED);
}
static void onLowMemory(ANativeActivity *activity) {
	push_app_command(APP_CMD_LOW_MEMORY);
}
static void onNativeWindowRedrawNeeded(ANativeActivity *activity, ANativeWindow *window) {
	push_app_command(APP_CMD_WINDOW_REDRAW_NEEDED);
}
static void onNativeWindowResized(ANativeActivity *activity, ANativeWindow *window) {
	push_app_command(APP_CMD_WINDOW_RESIZED);
}
static void onPause(ANativeActivity *activity) {
	push_app_command(APP_CMD_PAUSE);
}
static void onResume(ANativeActivity *activity) {
	push_app_command(APP_CMD_RESUME);
}
static void onStart(ANativeActivity *activity) {
	push_app_command(APP_CMD_START);
}
static void onStop(ANativeActivity *activity) {
	push_app_command(APP_CMD_STOP);
}
static void onDestroy(ANativeActivity *activity) {
	push_app_command(APP_CMD_DESTROY);
}
static void onWindowFocusChanged(ANativeActivity *activity, int hasFocus) {
	push_app_command(hasFocus ? APP_CMD_GAINED_FOCUS : APP_CMD_LOST_FOCUS);
}

static void onContentRectChanged(ANativeActivity *activity, const ARect *rect) {
	app_global.pending_rect = *rect;
	push_app_command(APP_CMD_CONTENT_RECT_CHANGED);
}

static void *onSaveInstanceState(ANativeActivity *activity, size_t *outSize) {
	//*outSize = sizeof(app_global.state);
	//return &app_global.state;
	return nullptr;
}

JNIEXPORT void ANativeActivity_onCreate(ANativeActivity *activity, void *savedState, size_t savedStateSize)
{
	struct ANativeActivityCallbacks *cb = activity->callbacks;

	cb->onConfigurationChanged = onConfigurationChanged;
	cb->onContentRectChanged = onContentRectChanged;
	cb->onDestroy = onDestroy;
	cb->onInputQueueCreated = onInputQueueCreated;
	cb->onInputQueueDestroyed = onInputQueueDestroyed;
	cb->onLowMemory = onLowMemory;
	cb->onNativeWindowCreated = onNativeWindowCreated;
	cb->onNativeWindowDestroyed = onNativeWindowDestroyed;
	cb->onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
	cb->onNativeWindowResized = onNativeWindowResized;
	cb->onPause = onPause;
	cb->onResume = onResume;
	cb->onSaveInstanceState = onSaveInstanceState;
	cb->onStart = onStart;
	cb->onStop = onStop;
	cb->onWindowFocusChanged = onWindowFocusChanged;

	app_global.activity = activity;
	activity->instance = &app_global;

	//if (savedState && savedStateSize == sizeof(app_global.state))
		//memcpy(&app_global.state, savedState, savedStateSize);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&app_global.thread, &attr, app_main, NULL);

	while (!app_global.running);
}