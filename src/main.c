// libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <threads.h>

// posix
#include <errno.h>

// gtk
#include <gtk/gtk.h>

// alsa / libasound
#include <alsa/asoundlib.h>

#include "config.h"

/// GTK
static void on_app_activate(GApplication *app, gpointer data);
static GtkWidget* create_main_window(GApplication* app);
static GtkWidget* create_alarm_label(const Config* config);

/// Alsa / libasound
typedef struct {
    const Config* config;
} SoundPlayerThreadContext;
static int sound_player_thread_start(void* thread_context);
static void play_wav_file(const char* file_path, unsigned int rate, int channels, int seconds);

int main(int argc, char *argv[])
{
    const Config config = config_from_env();

    // wait for the timeout
    while (true) {
        time_t t = time(NULL);
        struct tm current_time = *localtime(&t);
        if (current_time.tm_hour == config.hour && current_time.tm_min == config.min) {
            break;
        }
        sleep(60 - current_time.tm_sec);
    }

    thrd_t sound_player_thread;
    SoundPlayerThreadContext sound_player_thread_context = {
        .config = &config,
    };
    if (thrd_create(&sound_player_thread, sound_player_thread_start, &sound_player_thread_context) != thrd_success) {
        fprintf(stderr, "error: could not create sound player thread: %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

    GtkApplication *app = gtk_application_new(
        "br.com.hbobenicio.Alarm", 
        G_APPLICATION_FLAGS_NONE
    );
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), (gpointer) &config);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    if (thrd_join(sound_player_thread, NULL) != thrd_success) {
        fprintf(stderr, "error: can't join sound player thread: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return status;
}

static void on_app_activate(GApplication *app, gpointer data)
{
    const Config* config = (const Config*) data;

    GtkWidget* window = create_main_window(app);
    GtkWidget *alarm_label = create_alarm_label(config);

    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(alarm_label));
    gtk_widget_show_all(GTK_WIDGET(window));
}

static GtkWidget* create_main_window(GApplication* app)
{
    GtkWidget* window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
    return window;
}

static GtkWidget* create_alarm_label(const Config* config)
{
    GtkWidget *label = gtk_label_new(NULL);

    char* markup = g_markup_printf_escaped(
        "<span color=\"red\" size=\"xx-large\" weight=\"bold\">%s</span>",
        config->msg
    );
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);

    return label;
}

static int sound_player_thread_start(void* thread_context)
{
    (void) thread_context;
    // SoundPlayerThreadContext* ctx = (SoundPlayerThreadContext*) thread_context;

    play_wav_file("assets/alarm.wav", 44100, 1, 10);
    
    return 0;
}

static void play_wav_file(const char* file_path, unsigned int rate, int channels, int seconds)
{
    FILE* alarm_input_file = fopen(file_path, "rb");
    if (!alarm_input_file) {
        fprintf(stderr, "error: can't open alarm sound file at \"%s\": %s\n", file_path, strerror(errno));
        return;
    }

    int rc;

	// Open the PCM device in playback mode
	snd_pcm_t *pcm_handle = NULL;
    static const char* device_name = "default";
	if ((rc = snd_pcm_open(&pcm_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "error: can't open \"%s\" PCM device: %s\n", device_name, snd_strerror(rc));
        return;
    }

	// allocate an invalid snd_pcm_hw_params_t using standard alloca and fill it with default values
	snd_pcm_hw_params_t *params = NULL;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(pcm_handle, params);

	// set parameters: interleaved mode
	if ((rc = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "error: can't set interleaved mode. %s\n", snd_strerror(rc));
        snd_pcm_drain(pcm_handle);
	    snd_pcm_close(pcm_handle);
        return;
    }

    // set parameters: signed 16-bit little-endian format
	if ((rc = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf(stderr, "error: can't set format. %s\n", snd_strerror(rc));
        snd_pcm_drain(pcm_handle);
	    snd_pcm_close(pcm_handle);
        return;
    }

    // set parameters: channels
	if ((rc = snd_pcm_hw_params_set_channels(pcm_handle, params, channels)) < 0) {
		fprintf(stderr, "error: can't set channels number. %s\n", snd_strerror(rc));
        snd_pcm_drain(pcm_handle);
	    snd_pcm_close(pcm_handle);
        return;
    }

    //  set parameters: rating
	if ((rc = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0) {
		fprintf(stderr, "error: can't set rate. %s\n", snd_strerror(rc));
        snd_pcm_drain(pcm_handle);
	    snd_pcm_close(pcm_handle);
        return;
    }

	// write parameters
	if ((rc = snd_pcm_hw_params(pcm_handle, params)) < 0) {
		fprintf(stderr, "error: can't set harware parameters. %s\n", snd_strerror(rc));
        snd_pcm_drain(pcm_handle);
	    snd_pcm_close(pcm_handle);
        return;
    }

    unsigned int tmp;

	// Resume information
#ifdef DEBUG
	fprintf(stderr, "PCM name: '%s'\n", snd_pcm_name(pcm_handle));
	fprintf(stderr, "PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

	snd_pcm_hw_params_get_channels(params, &tmp);
	fprintf(stderr, "channels: %i ", tmp);

	if (tmp == 1)
		fprintf(stderr, "(mono)\n");
	else if (tmp == 2)
		fprintf(stderr, "(stereo)\n");

	snd_pcm_hw_params_get_rate(params, &tmp, 0);
	fprintf(stderr, "rate: %d bps\n", tmp);
	fprintf(stderr, "seconds: %d\n", seconds);	
#endif

	// Allocate buffer to hold single period
    snd_pcm_uframes_t frames;
	snd_pcm_hw_params_get_period_size(params, &frames, 0);

	size_t buff_size = frames * channels * 2; // 2 -> sample size
	char* buff = malloc(buff_size);

	snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

    // TODO what is this magic number?!
	for (int loops = (seconds * 1000000) / tmp; loops > 0; loops--) {
        // TODO error handling
        fread(buff, buff_size, 1, alarm_input_file);
		// if ((rc = read(0, buff, buff_size)) == 0) {
        //     fprintf(stderr, "early end of file.\n");
		// 	snd_pcm_drain(pcm_handle);
	    //     snd_pcm_close(pcm_handle);
		// 	free(buff);
		// 	return;
		// }

		if (snd_pcm_writei(pcm_handle, buff, frames) == -EPIPE) {
			fprintf(stderr, "XRUN.\n");
			snd_pcm_prepare(pcm_handle);
		} else if (rc < 0) {
			fprintf(stderr, "error. can't write to PCM device. %s\n", snd_strerror(rc));
		}
	}

	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
	free(buff);
}
