/*
 * Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Build with:
 *
 *    $ gcc -o nouveau_simplest main.c -ldrm_nouveau -lsync
 *
 * Depends on libsync.so being built and installed (source at ../libsync).
 */

#include <errno.h>
#include <fcntl.h>
#include <libdrm/nouveau.h>
#include <stdio.h>
#include <string.h>
#include <sw_sync.h>
#include <sync/sync.h>
#include <unistd.h>

static int sync_fence_status(int fd)
{
	int status;
	struct sync_fence_info_data *data;
	data = sync_fence_info(fd);
	if (!data)
		return -EINVAL;
	status = data->status;
	sync_fence_info_free(data);
	return status;
}

int main(int argc, char *argv[])
{
	int err, i, fd = -1, fence = -1, timeline;
	struct nouveau_device *dev = NULL;
	struct nouveau_client *client = NULL;
	struct nouveau_pushbuf *push = NULL;
	struct nouveau_object *chan = NULL, *threed = NULL;
	struct nvc0_fifo nvc0_data = { };
	bool use_fences = false;
	const char *devname = NULL;

	i = 1;
	while (i < argc) {
		const char *arg = argv[i++];
		if (!strcmp(arg, "--fence"))
			use_fences = true;
		else {
			devname = arg;
			break;
		}
	}

	if (i != argc || !devname) {
		printf("Usage: %s <drm-file>\n", argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		err = fd;
		printf("could not open %s: %s\n", argv[1], strerror(err));
		goto out;
	}

	err = nouveau_device_wrap(fd, 1, &dev);
	if (err) {
		printf("nouveau_device_open failed: %s\n", strerror(err));
		goto out;
	}

	err = nouveau_client_new(dev, &client);
	if (err) {
		printf("nouveau_client_new failed: %s\n", strerror(err));
		goto out;
	}

	err = nouveau_object_new(&dev->object, 0xbeef0001, NOUVEAU_FIFO_CHANNEL_CLASS,
				 &nvc0_data, sizeof(nvc0_data), &chan);
	if (err) {
		printf("nouveau_client_new failed for channel: %s\n", strerror(err));
		goto out;
	}

	err = nouveau_pushbuf_new(client, chan, 1, 4096, true, &push);
	if (err) {
		printf("nouveau_pushbuf_new failed: %s\n", strerror(err));
		goto out;
	}

	err = nouveau_object_new(chan, 0xbeef0002, 0xa297,
				 NULL, 0, &threed);
	if (err) {
		printf("nouveau_client_new failed for 3d: %s\n", strerror(err));
		goto out;
	}

	err = nouveau_pushbuf_space(push, 4, 0, 0);
	if (err) {
		printf("nouveau_pushbuf_space failed: %s\n", strerror(err));
		goto out;
	}

	*push->cur++ = 0x20010000; // SET_OBJECT
	*push->cur++ = 0x0000a297; // KEPLER_C
	*push->cur++ = 0x20010040; // NO_OPERATION
	*push->cur++ = 0x00000000;

	if (use_fences) {
#if 0
		timeline = sw_sync_timeline_create();
		if (timeline < 0) {
			err = timeline;
			printf("sw_sync_timeline_create failed: %s\n", strerror(err));
			printf("Do you have CONFIG_SW_SYNC_USER enabled in your kernel?\n");
			goto out;
		}

		fence = sw_sync_fence_create(timeline, argv[0], 1);
		if (fence < 0) {
			err = fence;
			printf("sw_sync_fence_create failed: %s\n", strerror(err));
			goto out;
		}
#endif

		err = nouveau_pushbuf_kick_fence(push, NULL, &fence);
		if (err) {
			printf("nouveau_pushbuf_kick_fence failed: %s\n", strerror(err));
			goto out;
		}

		printf("Status immediately after kick: %d\n", sync_fence_status(fence));

		// Sleep for 100ms to make sure that gpu gets stalled.
		printf("Sleeping...\n");
		usleep(100 * 1000);

		printf("Status after sleeping: %d\n", sync_fence_status(fence));

#if 0
		// Verify that the fence is still active.
		err = sync_fence_status(fence);
		if (err) {
			printf("fence status should be active but is %d\n", err);
			goto out;
		}

		// Signal the pre-fence so that gpu can start.
		err = sw_sync_timeline_inc(timeline, 1);
		if (err) {
			printf("sw_sync_timeline_inc failed: %s\n", strerror(err));
			goto out;
		}
#endif

		printf("Waiting for fence: %d\n", fence);
		err = sync_wait(fence, -1);
		if (err) {
			printf("sync_wait failed: %s\n", strerror(err));
			goto out;
		}
	} else {
		err = nouveau_pushbuf_kick(push, NULL);
		if (err) {
			printf("nouveau_pushbuf_kick failed: %s\n", strerror(err));
			goto out;
		}
		printf("nouveau_pushbuf_kick done, use --fence option to tell if it completed\n");
	}

out:
	nouveau_object_del(&threed);
	nouveau_pushbuf_del(&push);
	nouveau_object_del(&chan);
	nouveau_client_del(&client);
	if (dev)
		nouveau_device_del(&dev);
	else
		close(fd);
	if (fence >= 0)
		close(fence);
	return err;
}
