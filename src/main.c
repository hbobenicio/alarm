#ifdef __STDC_NO_THREADS__
#error "C11 threads.h support is required."
#endif

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
#include "wav-parser.h"

/// GTK
static void on_app_activate(GApplication *app, gpointer data);
static GtkWidget* create_main_window(GApplication* app);
static GtkWidget* create_alarm_label(const Config* config);

/// Alsa / libasound
typedef struct {
    const Config* config;
} SoundPlayerThreadContext;
static int sound_player_thread_start(void* thread_context);
static int play_wav_file(const char* file_path, unsigned int rate, int channels, int seconds);

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
        const char* posix_reason = strerror(errno);
        fprintf(stderr, "error: could not create sound player thread: %s.\n", posix_reason);
        return EXIT_FAILURE;
    }
    if (thrd_detach(sound_player_thread) != thrd_success) {
        const char* posix_reason = strerror(errno);
        fprintf(stderr, "error: failed to detach sound player thread: %s.\n", posix_reason);
        return EXIT_FAILURE;
    }

    GtkApplication *app = gtk_application_new(
        "br.com.hbobenicio.Alarm", 
        G_APPLICATION_FLAGS_NONE
    );
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), (gpointer) &config);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    // TODO maybe kill the sound player thread?

    // if (thrd_join(sound_player_thread, NULL) != thrd_success) {
    //     fprintf(stderr, "error: can't join sound player thread: %s\n", strerror(errno));
    //     return EXIT_FAILURE;
    // }

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
    SoundPlayerThreadContext* ctx = (SoundPlayerThreadContext*) thread_context;
    (void) ctx;

    //return play_wav_file("assets/alarm.wav", 44100, 1, 10);
    // play_wav_file("assets/acorda-fidaegua.ogg", 44100, 1, 10);
    return play_wav_file("assets/acorda-fidaegua.wav", 48000, 1, 10);
    //return play_wav_file("assets/abestado-seligai.wav", 48000, 1, 10);
}

static int play_wav_file(const char* file_path, unsigned int rate, int channels, int seconds)
{
    int rc;

	// Open the PCM device in playback mode
	snd_pcm_t *pcm_handle = NULL;
    static const char* device_name = "default";
	if ((rc = snd_pcm_open(&pcm_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "error: can't open \"%s\" PCM device: %s\n", device_name, snd_strerror(rc));
        return 1;
    }

	// allocate an invalid snd_pcm_hw_params_t using standard alloca and fill it with default values
	snd_pcm_hw_params_t *params = NULL;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(pcm_handle, params);

	// set parameters: interleaved mode
	if ((rc = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "error: can't set interleaved mode. %s\n", snd_strerror(rc));
        goto cleanup_pcm;
    }

    // set parameters: signed 16-bit little-endian format
	if ((rc = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf(stderr, "error: can't set format. %s\n", snd_strerror(rc));
        goto cleanup_pcm;
    }

    // set parameters: channels
	if ((rc = snd_pcm_hw_params_set_channels(pcm_handle, params, channels)) < 0) {
		fprintf(stderr, "error: can't set channels number. %s\n", snd_strerror(rc));
        goto cleanup_pcm;
    }

    //  set parameters: rating
	if ((rc = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0) {
		fprintf(stderr, "error: can't set rate. %s\n", snd_strerror(rc));
        goto cleanup_pcm;
    }

	// write parameters
	if ((rc = snd_pcm_hw_params(pcm_handle, params)) < 0) {
		fprintf(stderr, "error: can't set harware parameters. %s\n", snd_strerror(rc));
        goto cleanup_pcm;
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

    FILE* alarm_input_file = fopen(file_path, "rb");
    if (!alarm_input_file) {
        fprintf(stderr, "error: can't open alarm sound file at \"%s\": %s\n", file_path, strerror(errno));
        goto cleanup_pcm_buff;
    }
    // if (fseek(alarm_input_file, 40, SEEK_CUR) == -1) {
    //     fprintf(stderr, "error: can't seek alarm sound file \"%s\" past the wav header: %s\n", file_path, strerror(errno));
    //     goto cleanup_pcm_buff_file;
    // }
    WAVParser wav_parser = wav_parser_from_file(alarm_input_file);
    WAVHeader wav_header = wav_parser_parse_header(&wav_parser);
#ifdef DEBUG
    fputs("info: wav:\n", stderr);
    {
        fputs(" riff:\n", stderr);

        char s[5] = {0};
        memcpy(s, &wav_header.riff.id, sizeof(uint32_t));
        fprintf(stderr, "  id: \"%s\"\n", s);
        fprintf(stderr, "  size: %u\n", wav_header.riff.size);
        fprintf(stderr, "  format: %u\n", wav_header.riff.format);
    }
#endif

    // TODO what is this magic number?!
	for (int loops = (seconds * 1000000) / tmp; loops > 0; loops--) {
        // TODO error handling
        fread(buff, buff_size, 1, alarm_input_file);
        if ((rc = ferror(alarm_input_file)) != 0) {
            fprintf(stderr, "error: failed to read alarm sound stream.\n");
            goto cleanup_pcm_buff_file;
        }
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

    fclose(alarm_input_file);
	free(buff);
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
    return 0;

cleanup_pcm_buff_file:
    fclose(alarm_input_file);
cleanup_pcm_buff:
    free(buff);
cleanup_pcm:
    snd_pcm_drop(pcm_handle);
    snd_pcm_close(pcm_handle);
    return 1;
}
