/*
 * Copyright (C) 2013 Paul Kocialkowski <contact@paulk.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <linux/ioctl.h>
#include <linux/input.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "wave_sensors"
#include <utils/Log.h>

#include "wave_sensors.h"

struct smb380_data {
	char path_enable[PATH_MAX];
	char path_delay[PATH_MAX];

	sensors_vec_t acceleration;
};

int smb380_init(struct wave_sensors_handlers *handlers,
	struct wave_sensors_device *device)
{
	struct smb380_data *data = NULL;
	char path[PATH_MAX] = { 0 };
	int input_fd = -1;
	int rc;

	ALOGD("%s(%p, %p)", __func__, handlers, device);

	if (handlers == NULL)
		return -EINVAL;

	data = (struct smb380_data *) calloc(1, sizeof(struct smb380_data));

	input_fd = input_open("accelerometer_sensor");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open input", __func__);
		goto error;
	}

	rc = sysfs_path_prefix("accelerometer_sensor", (char *) &path);
	if (rc < 0 || path[0] == '\0') {
		ALOGE("%s: Unable to open sysfs", __func__);
		goto error;
	}

	snprintf(data->path_enable, PATH_MAX, "%s/enable", path);
	snprintf(data->path_delay, PATH_MAX, "%s/delay", path);

	handlers->poll_fd = input_fd;
	handlers->data = (void *) data;

	return 0;

error:
	if (data != NULL)
		free(data);

	if (input_fd >= 0)
		close(input_fd);

	handlers->poll_fd = -1;
	handlers->data = NULL;

	return -1;
}

int smb380_deinit(struct wave_sensors_handlers *handlers)
{
	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL)
		return -EINVAL;

	if (handlers->poll_fd >= 0)
		close(handlers->poll_fd);
	handlers->poll_fd = -1;

	if (handlers->data != NULL)
		free(handlers->data);
	handlers->data = NULL;

	return 0;
}

int smb380_activate(struct wave_sensors_handlers *handlers)
{
	struct smb380_data *data;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct smb380_data *) handlers->data;

	rc = sysfs_value_write(data->path_enable, 1);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	handlers->activated = 1;

	return 0;
}

int smb380_deactivate(struct wave_sensors_handlers *handlers)
{
	struct smb380_data *data;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct smb380_data *) handlers->data;

	rc = sysfs_value_write(data->path_enable, 0);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	handlers->activated = 1;

	return 0;
}

int smb380_set_delay(struct wave_sensors_handlers *handlers, long int delay)
{
	struct smb380_data *data;
	int d;
	int rc;

	ALOGD("%s(%p, %ld)", __func__, handlers, delay);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct smb380_data *) handlers->data;

	if (delay < 10000000)
		d = 10;
	else
		d = delay / 1000000;

	rc = sysfs_value_write(data->path_delay, d);
	if (rc < 0) {
		ALOGE("%s: Unable to write sysfs value", __func__);
		return -1;
	}

	return 0;
}

float smb380_convert(int value)
{
	return value * (GRAVITY_EARTH / 256.0f);
}

int smb380_get_data(struct wave_sensors_handlers *handlers,
	struct sensors_event_t *event)
{
	struct smb380_data *data;
	struct input_event input_event;
	int input_fd;
	int rc;

//	ALOGD("%s(%p, %p)", __func__, handlers, event);

	if (handlers == NULL || handlers->data == NULL || event == NULL)
		return -EINVAL;

	data = (struct smb380_data *) handlers->data;

	input_fd = handlers->poll_fd;
	if (input_fd < 0)
		return -EINVAL;

	memset(event, 0, sizeof(struct sensors_event_t));
	event->version = sizeof(struct sensors_event_t);
	event->sensor = handlers->handle;
	event->type = handlers->handle;

	event->acceleration.x = data->acceleration.x;
	event->acceleration.y = data->acceleration.y;
	event->acceleration.z = data->acceleration.z;

	do {
		rc = read(input_fd, &input_event, sizeof(input_event));
		if (rc < (int) sizeof(input_event))
			break;

		if (input_event.type == EV_REL) {
			switch (input_event.code) {
				case REL_X:
					event->acceleration.x = smb380_convert(input_event.value * -1);
					break;
				case REL_Y:
					event->acceleration.y = smb380_convert(input_event.value * -1);
					break;
				case REL_Z:
					event->acceleration.z = smb380_convert(input_event.value);
					break;
				default:
					continue;
			}
		} else if (input_event.type == EV_SYN) {
			if (input_event.code == SYN_REPORT)
				event->timestamp = input_timestamp(&input_event);
		}
	} while (input_event.type != EV_SYN);

	data->acceleration.x = event->acceleration.x;
	data->acceleration.y = event->acceleration.y;
	data->acceleration.z = event->acceleration.z;

	return 0;
}

struct wave_sensors_handlers smb380 = {
	.name = "SMB380",
	.handle = SENSOR_TYPE_ACCELEROMETER,
	.init = smb380_init,
	.deinit = smb380_deinit,
	.activate = smb380_activate,
	.deactivate = smb380_deactivate,
	.set_delay = smb380_set_delay,
	.get_data = smb380_get_data,
	.activated = 0,
	.needed = 0,
	.poll_fd = -1,
	.data = NULL,
};
