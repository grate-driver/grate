/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Thierry Reding
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <time.h>

#include "libgrate-private.h"

struct grate_profile {
	struct timespec start;
	struct timespec end;
	unsigned int frames;
};

static float timespec_diff(const struct timespec *start,
			   const struct timespec *end)
{
	unsigned long seconds = end->tv_sec - start->tv_sec;
	long ns = end->tv_nsec - start->tv_nsec;

	if (ns < 0) {
		ns += 1000000000;
		seconds--;
	}

	return seconds + ns / 1000000000.0f;
}

static void grate_profile_dump(struct grate_profile *profile, FILE *fp)
{
	float time = timespec_diff(&profile->start, &profile->end);

	fprintf(fp, "%u frames in %.3f seconds: %.2f fps\n", profile->frames,
		time, profile->frames / time);
}

struct grate_profile *grate_profile_start(struct grate *grate)
{
	struct grate_profile *profile;

	profile = calloc(1, sizeof(*profile));
	if (!profile)
		return NULL;

	clock_gettime(CLOCK_MONOTONIC, &profile->start);
	profile->frames = 0;

	return profile;
}

void grate_profile_free(struct grate_profile *profile)
{
	free(profile);
}

void grate_profile_sample(struct grate_profile *profile)
{
	profile->frames++;
}

void grate_profile_finish(struct grate_profile *profile)
{
	clock_gettime(CLOCK_MONOTONIC, &profile->end);
	grate_profile_dump(profile, stdout);
}

float grate_profile_time_elapsed(struct grate_profile *profile)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);

	return timespec_diff(&profile->start, &now);
}
