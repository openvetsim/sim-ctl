/*
 * simGpio.cpp
 *
 * This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
 *
 * Copyright (c) 2019-2026 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * GPIO implementation updated to use libgpiod in place of the legacy
 * sysfs /sys/class/gpio interface.
 *
 * BeagleBone GPIO numbering convention:
 *   sysfs pin  =  chip * 32 + line_offset
 *   e.g. pin 49  ->  chip 1, line 17  (/dev/gpiochip1, offset 17)
 *
 * Build requirement:
 *   Link with -lgpiod  (apt install libgpiod-dev)
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <gpiod.h>

#include "simUtil.h"
#include "shmData.h"

// GPIO Access (kept for callers that test direction)
#define GPIO_TURN_ON	1
#define GPIO_TURN_OFF	0
#define GPIO_INPUT		1
#define GPIO_OUTPUT		0

#define GPIO_CONSUMER	"sim-ctl"

extern int debug;

/**
 * pin_to_chip_line
 *
 * Convert a legacy sysfs GPIO pin number to a libgpiod chip number and
 * line offset using the BeagleBone convention: chip = pin / 32,
 * line_offset = pin % 32.
*/
static void
pin_to_chip_line(int pin, unsigned int *chip_num, unsigned int *line_offset)
{
	*chip_num    = (unsigned int)(pin / 32);
	*line_offset = (unsigned int)(pin % 32);
}

/**
 * gpioPinOpen
 *
 * Open a GPIO line and configure its direction.
 * Returns a struct gpiod_line * that must be passed to gpioPinSet /
 * gpioPinGet, or NULL on error.
 *
 * The caller is responsible for releasing the line and closing the chip
 * when it is no longer needed:
 *   gpiod_line_release(line);
 *   gpiod_chip_close(gpiod_line_get_chip(line));
*/
struct gpiod_line *
gpioPinOpen(int pin, int direction)
{
	unsigned int chip_num, line_offset;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int ret;

	if ( debug )
	{
		printf("gpioPinOpen(%d, %d)\n", pin, direction);
	}

	pin_to_chip_line(pin, &chip_num, &line_offset);

	chip = gpiod_chip_open_by_number(chip_num);
	if (!chip) {
		fprintf(stderr, "gpioPinOpen: gpiod_chip_open_by_number(%u) failed: %s\n",
		        chip_num, strerror(errno));
		return NULL;
	}

	line = gpiod_chip_get_line(chip, line_offset);
	if (!line) {
		fprintf(stderr, "gpioPinOpen: gpiod_chip_get_line(offset %u) failed: %s\n",
		        line_offset, strerror(errno));
		gpiod_chip_close(chip);
		return NULL;
	}

	if (direction == GPIO_OUTPUT) {
		ret = gpiod_line_request_output(line, GPIO_CONSUMER, 0);
	} else {
		ret = gpiod_line_request_input(line, GPIO_CONSUMER);
	}

	if (ret < 0) {
		fprintf(stderr, "gpioPinOpen: gpiod_line_request_%s failed: %s\n",
		        (direction == GPIO_OUTPUT) ? "output" : "input",
		        strerror(errno));
		gpiod_chip_close(chip);
		return NULL;
	}

	return line;
}

/**
 * gpioPinSet
 *
 * Drive the GPIO line high (val != 0) or low (val == 0).
*/
void
gpioPinSet(struct gpiod_line *line, int val)
{
	if (val != 0) {
		val = 1;
	}

	if (gpiod_line_set_value(line, val) < 0) {
		fprintf(stderr, "gpioPinSet: gpiod_line_set_value(%d) failed: %s\n",
		        val, strerror(errno));
	}
}

/**
 * gpioPinRead
 *
 * One-shot read of a GPIO input line by pin number.
 * Opens the chip, samples the line, then releases everything.
 * Returns 0 on success, -1 on error.
*/
int
gpioPinRead(int pin, int *value)
{
	unsigned int chip_num, line_offset;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int ret;

	pin_to_chip_line(pin, &chip_num, &line_offset);

	chip = gpiod_chip_open_by_number(chip_num);
	if (!chip) {
		fprintf(stderr, "gpioPinRead: gpiod_chip_open_by_number(%u) failed: %s\n",
		        chip_num, strerror(errno));
		return -1;
	}

	line = gpiod_chip_get_line(chip, line_offset);
	if (!line) {
		fprintf(stderr, "gpioPinRead: gpiod_chip_get_line(offset %u) failed: %s\n",
		        line_offset, strerror(errno));
		gpiod_chip_close(chip);
		return -1;
	}

	ret = gpiod_line_request_input(line, GPIO_CONSUMER);
	if (ret < 0) {
		fprintf(stderr, "gpioPinRead: gpiod_line_request_input failed: %s\n",
		        strerror(errno));
		gpiod_chip_close(chip);
		return -1;
	}

	ret = gpiod_line_get_value(line);

	gpiod_line_release(line);
	gpiod_chip_close(chip);

	if (ret < 0) {
		fprintf(stderr, "gpioPinRead: gpiod_line_get_value failed: %s\n",
		        strerror(errno));
		return -1;
	}

	*value = ret;
	return 0;
}

/**
 * gpioPinGet
 *
 * Read the current value of a GPIO input line that was previously opened
 * with gpioPinOpen().  Returns 0 on success, -1 on error.
*/
int
gpioPinGet(struct gpiod_line *line, int *value)
{
	int ret = gpiod_line_get_value(line);
	if (ret < 0) {
		fprintf(stderr, "gpioPinGet: gpiod_line_get_value failed: %s\n",
		        strerror(errno));
		return -1;
	}
	*value = ret;
	return 0;
}
