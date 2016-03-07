/* tinyplay.c
 **
 ** Copyright 2011, The Android Open Source Project
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are met:
 **     * Redistributions of source code must retain the above copyright
 **       notice, this list of conditions and the following disclaimer.
 **     * Redistributions in binary form must reproduce the above copyright
 **       notice, this list of conditions and the following disclaimer in the
 **       documentation and/or other materials provided with the distribution.
 **     * Neither the name of The Android Open Source Project nor the names of
 **       its contributors may be used to endorse or promote products derived
 **       from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
 ** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 ** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
 ** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 ** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 ** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 ** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 ** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 ** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 ** DAMAGE.
 */

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#define ID_RIFF *(uint32_t *) "RIFF"
#define ID_WAVE *(uint32_t *) "WAVE"
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
	uint32_t riff_id; /* Archivo tipo RIFF */
	uint32_t riff_sz; /* Tamaño de RIFF (no lo utilizaremos) */
	uint32_t wave_id; /* Formato WAVE */
};

struct chunk_header {
	uint32_t id; /* Tipo de chunk, FMT o DATA */
	uint32_t sz; /* Tamaño del chunk */
};

struct chunk_fmt {
	uint16_t audio_format; /* Formato de audio: PCM = 1 */
	uint16_t num_channels; /* Número de canales */
	uint32_t sample_rate;  /* Frecuencia de muestreo */
	uint32_t byte_rate;    /* Frecuencia de bits */
	uint16_t block_align;  /* Alineacion */
	uint16_t bits_per_sample; /* bits por cada muestra */
};

struct pcm_params {
	unsigned int card;
	unsigned int device;
	unsigned int channels;
	unsigned int rate;
	unsigned int bits;
	unsigned int period_size;
	unsigned int period_count;
};

/* variables globales */
static int close = 0;
static struct pcm_params par;


void set_param(unsigned int card, unsigned int device, unsigned int channels,
		unsigned int rate, unsigned int bits, unsigned int period_size,
		unsigned int period_count);

void play_sample(FILE * file);

/* funcion que recibe la señal */
void stream_close(int sig)
{
}

int main(int argc, char **argv)
{
	/* variables locales */
	FILE *file;
	/* ... */

	if (argc < 2) {
		fprintf(stderr, "Usage1: %s file.wav [-D card] [-d device] [-p period_size]"
				" [-n n_periods] \n", argv[0]);
		fprintf(stderr, "Usage2: %s file.raw [-D card] [-d device] [-p period_size] "
				"[-n n_periods] [-c channels] [-r rate] [-b bits] -i raw \n", argv[0]);
		return 1;
	}

	filename = argv[1];
	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		return 1;
	}

	/* parsear los argumentos de la linea de commandos */
	argv += 2;
	while (*argv) {
		if (strcmp(*argv, "-d") == 0) {
			argv++;
			if (*argv)
				device = atoi(*argv);
		}
		if (strcmp(*argv, "-p") == 0) {
			argv++;
			if (*argv)
				period_size = atoi(*argv);
		}
		if (strcmp(*argv, "-n") == 0) {
			argv++;
			if (*argv)
				period_count = atoi(*argv);
		}
		if (strcmp(*argv, "-D") == 0) {
			argv++;
			if (*argv)
				card = atoi(*argv);
		}
		if (strcmp(*argv, "-c") == 0) {
			argv++;
			if (*argv)
				channels = atoi(*argv);
		}
		if (strcmp(*argv, "-r") == 0) {
			argv++;
			if (*argv)
				rate = atoi(*argv);
		}
		if (strcmp(*argv, "-b") == 0) {
			argv++;
			if (*argv)
				bits = atoi(*argv);
		}
		if (strcmp(*argv, "-i") == 0) {
			argv++;
			if (*argv) {
				if (strcasecmp(*argv, "raw") == 0) {
					is_raw = 1;
				}
			}
		}
		if (*argv)
			argv++;
	}

	/* codigo para leer los encabezados y obtener los parametros */

	/* configurar parametros y hacer play */

	/* cerrar archivo */
	return 0;
}

/* revisa si los parametros utilizados son soportados por la tarjeta de sonido */
int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
		char *param_name, char *param_unit)
{
	unsigned int min;
	unsigned int max;
	int is_within_bounds = 1;

	min = pcm_params_get_min(params, param);
	if (value < min) {
		fprintf(stderr, "%s is %u%s, device only supports >= %u%s\n", param_name, value,
				param_unit, min, param_unit);
		is_within_bounds = 0;
	}

	max = pcm_params_get_max(params, param);
	if (value > max) {
		fprintf(stderr, "%s is %u%s, device only supports <= %u%s\n", param_name, value,
				param_unit, max, param_unit);
		is_within_bounds = 0;
	}

	return is_within_bounds;
}

int sample_is_playable()
{
	struct pcm_params *params;
	int can_play;

	params = pcm_params_get(par.card, par.device, PCM_OUT);
	if (params == NULL) {
		fprintf(stderr, "Unable to open PCM device %u.\n", par.device);
		return 0;
	}

	can_play = check_param(params, PCM_PARAM_RATE, par.rate, "Sample rate", "Hz");
	can_play &= check_param(params, PCM_PARAM_CHANNELS, par.channels, "Sample", " channels");
	can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, par.bits, "Bitrate", " bits");
	can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, par.period_size, "Period size", "Hz");
	can_play &= check_param(params, PCM_PARAM_PERIODS, par.period_count, "Period count", "Hz");

	pcm_params_free(params);

	return can_play;
}

/* funcion para reproducir el archivo */
void play_sample(FILE * file)
{
	/* ... */
}

/* configurar los parametros */
void set_param(unsigned int card, unsigned int device, unsigned int channels,
		unsigned int rate, unsigned int bits, unsigned int period_size,
		unsigned int period_count)
{
	par.card = card;
	par.device = device;
	par.channels = channels;
	par.rate = rate;
	par.bits = bits;
	par.period_size = period_size;
	par.period_count = period_count;
}

