/*
 * BlueALSA - ctl-client.c
 * Copyright (c) 2016-2018 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "ctl-client.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include "slog.h"

#if 1
	# define debug pr_debug
#else
	# define debug(M, ARGS...) do {} while (0)
#endif

/**
 * Convert BlueALSA status message into the POSIX errno value. */
static int bluealsa_status_to_errno(const struct ba_msg_status *status) {
	switch (status->code) {
	case BA_STATUS_CODE_SUCCESS:
		return 0;
	case BA_STATUS_CODE_ERROR_UNKNOWN:
		return EIO;
	case BA_STATUS_CODE_DEVICE_NOT_FOUND:
		return ENODEV;
	case BA_STATUS_CODE_STREAM_NOT_FOUND:
		return ENXIO;
	case BA_STATUS_CODE_DEVICE_BUSY:
		return EBUSY;
	case BA_STATUS_CODE_FORBIDDEN:
		return EACCES;
	default:
		/* some generic error code */
		return EINVAL;
	}
}

#ifdef DEBUG
/**
 * Convert Bluetooth address into a human-readable string.
 *
 * In order to convert Bluetooth address into the human-readable string, one
 * might use the ba2str() from the bluetooth library. However, it would be the
 * only used function from this library. In order to avoid excessive linking,
 * we are providing our own implementation of this function.
 *
 * @param ba Pointer to the Bluetooth address structure.
 * @param str Pointer to buffer big enough to contain string representation
 *   of the Bluetooth address.
 * @return Pointer to the destination string str. */
static char *ba2str_(const bdaddr_t *ba, char str[18]) {
	sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
			ba->b[5], ba->b[4], ba->b[3], ba->b[2], ba->b[1], ba->b[0]);
	return str;
}
#endif

/**
 * Send request to the BlueALSA server.
 *
 * @param fd Opened socket file descriptor.
 * @param req An address to the request structure.
 * @return Upon success this function returns 0. Otherwise, -1 is returned
 *   and errno is set appropriately. */
static int bluealsa_send_request(int fd, const struct ba_request *req) {
	struct ba_msg_status status;

	status.code = 0xAB;
	if (send(fd, req, sizeof(*req), MSG_NOSIGNAL) == -1)
		return -1;
	if (read(fd, &status, sizeof(status)) == -1)
		return -1;

	errno = bluealsa_status_to_errno(&status);
	return errno != 0 ? -1 : 0;
}

/**
 * Open BlueALSA connection.
 *
 * @param interface HCI interface to use.
 * @return On success this function returns socket file descriptor. Otherwise,
 *   -1 is returned and errno is set to indicate the error. */
int bluealsa_open(const char *interface) {
	const uint16_t ver = BLUEALSA_CRL_PROTO_VERSION;
	int fd, err;

	struct sockaddr_un saddr;
	saddr.sun_family = AF_UNIX;

	snprintf(saddr.sun_path, sizeof(saddr.sun_path) - 1,
			BLUEALSA_RUN_STATE_DIR "/%s", interface);

	if ((fd = socket(PF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0)) == -1)
		return -1;

	debug("Connecting to socket: %s\n", saddr.sun_path);
	if (connect(fd, (struct sockaddr *)(&saddr), sizeof(saddr)) == -1) {
		err = errno;
		close(fd);
		errno = err;
		return -1;
	}

	if (send(fd, &ver, sizeof(ver), MSG_NOSIGNAL) == -1)
		return -1;

	return fd;
}

/**
 * Subscribe for notifications.
 *
 * @param fd Opened socket file descriptor.
 * @param mask Bit-mask with events for which client wants to be subscribed.
 *   In order to cancel subscription, use empty event mask.
 * @return Upon success this function returns 0. Otherwise, -1 is returned
 *   and errno is set appropriately. */
int bluealsa_subscribe(int fd, enum ba_event mask) {
	struct ba_request req;

	req.command = BA_COMMAND_SUBSCRIBE;
	req.events = mask;

	debug("Subscribing for events: %d\n", mask);
	return bluealsa_send_request(fd, &req);
}

/**
 * Get the list of connected Bluetooth devices.
 *
 * @param fd Opened socket file descriptor.
 * @param devices An address where the device list will be stored.
 * @return Upon success this function returns the number of connected devices
 *   and the `devices` address is modified to point to the devices list array,
 *   which should be freed with the free(). On error, -1 is returned and errno
 *   is set to indicate the error. */
ssize_t bluealsa_get_devices(int fd, struct ba_msg_device **devices) {
	struct ba_request req;
	struct ba_msg_device *_devices = NULL;
	struct ba_msg_device device;
	size_t i = 0;

	req.command = BA_COMMAND_LIST_DEVICES;
	if (send(fd, &req, sizeof(req), MSG_NOSIGNAL) == -1)
		return -1;

	while (recv(fd, &device, sizeof(device), 0) == sizeof(device)) {
		_devices = (struct ba_msg_device *)realloc(_devices, (i + 1) * sizeof(*_devices));
		memcpy(&_devices[i], &device, sizeof(*_devices));
		i++;
	}

	*devices = _devices;
	return i;
}

/**
 * Get the list of available PCM transports.
 *
 * @param fd Opened socket file descriptor.
 * @param transports An address where the transport list will be stored.
 * @return Upon success this function returns the number of available PCM
 *   transports and the `transports` address is modified to point to the
 *   transport list array, which should be freed with the free(). On error,
 *   -1 is returned and errno is set to indicate the error. */
ssize_t bluealsa_get_transports(int fd, struct ba_msg_transport **transports) {
	struct ba_request req;
	struct ba_msg_transport *_transports = NULL;
	struct ba_msg_transport transport;
	size_t i = 0;

	req.command = BA_COMMAND_LIST_TRANSPORTS;
	if (send(fd, &req, sizeof(req), MSG_NOSIGNAL) == -1)
		return -1;

	while (recv(fd, &transport, sizeof(transport), 0) == sizeof(transport)) {
		_transports = (struct ba_msg_transport *)realloc(_transports, (i + 1) * sizeof(*_transports));
		memcpy(&_transports[i], &transport, sizeof(*_transports));
		i++;
	}

	*transports = _transports;
	return i;
}

/**
 * Get PCM transport.
 *
 * @param fd Opened socket file descriptor.
 * @param addr MAC address of the Bluetooth device.
 * @param type PCM type to get.
 * @param stream Stream direction to get, e.g. playback or capture.
 * @param transport An address where the transport will be stored.
 * @return Upon success this function returns 0. Otherwise, -1 is returned
 *   and errno is set appropriately. */
int bluealsa_get_transport(int fd, bdaddr_t addr, enum ba_pcm_type type,
		enum ba_pcm_stream stream, struct ba_msg_transport *transport) {
	struct ba_msg_status status = { 0xAB };
	struct ba_request req;
	ssize_t len;

	req.command = BA_COMMAND_TRANSPORT_GET;
	req.addr = addr;
	req.type = type;
	req.stream = stream;

#ifdef DEBUG
	char addr_[18];
	ba2str_(&req.addr, addr_);
	debug("Getting transport for %s type %d\n", addr_, type);
#endif

	if (send(fd, &req, sizeof(req), MSG_NOSIGNAL) == -1)
		return -1;
	if ((len = read(fd, transport, sizeof(*transport))) == -1)
		return -1;

	/* in case of error, status message is returned */
	if (len != sizeof(*transport)) {
		memcpy(&status, transport, sizeof(status));
		errno = bluealsa_status_to_errno(&status);
		return -1;
	}

	if (read(fd, &status, sizeof(status)) == -1)
		return -1;

	return 0;
}

/**
 * Get PCM transport delay.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @param delay An address where the transport delay will be stored.
 * @return Upon success this function returns 0. Otherwise, -1 is returned
 *   and errno is set appropriately. */
int bluealsa_get_transport_delay(int fd, const struct ba_msg_transport *transport,
		unsigned int *delay) {
	struct ba_msg_transport t;
	int ret;

	if ((ret = bluealsa_get_transport(fd, transport->addr,
					transport->type, transport->stream, &t)) == 0)
		*delay = t.delay;

	return ret;
}

/**
 * Set PCM transport delay.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @param delay Transport delay.
 * @return Upon success this function returns 0. Otherwise, -1 is returned
 *   and errno is set appropriately. */
int bluealsa_set_transport_delay(int fd, const struct ba_msg_transport *transport,
		unsigned int delay) {
	struct ba_request req;

	req.command = BA_COMMAND_TRANSPORT_SET_DELAY;
	req.addr = transport->addr;
	req.type = transport->type;
	req.stream = transport->stream;
	req.delay = delay;

	return bluealsa_send_request(fd, &req);
}

/**
 * Get PCM transport volume.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @param ch1_muted An address where the mute of channel 1 will be stored.
 * @param ch1_volume An address where the volume of channel 1 will be stored.
 * @param ch2_muted An address where the mute of channel 2 will be stored.
 * @param ch2_volume An address where the volume of channel 2 will be stored.
 * @return Upon success this function returns 0. Otherwise, -1 is returned
 *   and errno is set appropriately. */
int bluealsa_get_transport_volume(int fd, const struct ba_msg_transport *transport,
		bool *ch1_muted, int *ch1_volume, bool *ch2_muted, int *ch2_volume) {
	struct ba_msg_transport t;
	int ret;

	if ((ret = bluealsa_get_transport(fd, transport->addr,
					transport->type, transport->stream, &t)) == 0) {
		*ch1_muted = t.ch1_muted;
		*ch1_volume = t.ch1_volume;
		*ch2_muted = t.ch2_muted;
		*ch2_volume = t.ch2_volume;
	}

	return ret;
}

/**
 * Set PCM transport volume.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @param ch1_muted If true, mute channel 1.
 * @param ch1_volume Channel 1 volume in range [0, 127].
 * @param ch2_muted If true, mute channel 2.
 * @param ch2_volume Channel 2 volume in range [0, 127].
 * @return Upon success this function returns 0. Otherwise, -1 is returned
 *   and errno is set appropriately. */
int bluealsa_set_transport_volume(int fd, const struct ba_msg_transport *transport,
		bool ch1_muted, int ch1_volume, bool ch2_muted, int ch2_volume) {
	struct ba_request req;

	req.command = BA_COMMAND_TRANSPORT_SET_VOLUME;
	req.addr = transport->addr;
	req.type = transport->type;
	req.stream = transport->stream;
	req.ch1_muted = ch1_muted;
	req.ch1_volume = ch1_volume;
	req.ch2_muted = ch2_muted;
	req.ch2_volume = ch2_volume;

	return bluealsa_send_request(fd, &req);
}

/**
 * Open PCM transport.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @return PCM FIFO file descriptor, or -1 on error. */
int bluealsa_open_transport(int fd, const struct ba_msg_transport *transport) {
	struct ba_msg_status status;
	struct ba_request req;
	char buf[256] = "";
	struct iovec io;
	struct msghdr msg;
	ssize_t len;

#ifdef DEBUG
	char addr_[18];
	ba2str_(&req.addr, addr_);
	debug("Requesting PCM open for %s\n", addr_);
#endif

	status.code = 0xAB;

	req.command = BA_COMMAND_PCM_OPEN;
	req.addr = transport->addr;
	req.type = transport->type;
	req.stream = transport->stream;

	io.iov_base = &status;
	io.iov_len = sizeof(status);

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	if (send(fd, &req, sizeof(req), MSG_NOSIGNAL) == -1)
		return -1;
	if ((len = recvmsg(fd, &msg, MSG_CMSG_CLOEXEC)) == -1)
		return -1;

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL ||
			cmsg->cmsg_level == IPPROTO_IP ||
			cmsg->cmsg_type == IP_TTL) {
		/* in case of error, status message is returned */
		errno = bluealsa_status_to_errno(&status);
		return -1;
	}

	if (read(fd, &status, sizeof(status)) == -1)
		return -1;

	return *((int *)CMSG_DATA(cmsg));
}

/**
 * Close PCM transport.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @return Upon success this function returns 0. Otherwise, -1 is returned. */
int bluealsa_close_transport(int fd, const struct ba_msg_transport *transport) {
	struct ba_request req;

#ifdef DEBUG
	char addr_[18];
	ba2str_(&req.addr, addr_);
	debug("Closing PCM for %s\n", addr_);
#endif

	req.command = BA_COMMAND_PCM_CLOSE;
	req.addr = transport->addr;
	req.type = transport->type;
	req.stream = transport->stream;

	return bluealsa_send_request(fd, &req);
}

/**
 * Pause/resume PCM transport.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @param pause If non-zero, pause transport, otherwise resume it.
 * @return Upon success this function returns 0. Otherwise, -1 is returned. */
int bluealsa_pause_transport(int fd, const struct ba_msg_transport *transport, bool pause) {
	struct ba_request req;

#ifdef DEBUG
	char addr_[18];
	ba2str_(&req.addr, addr_);
	debug("Requesting PCM %s for %s\n", pause ? "pause" : "resume", addr_);
#endif

	req.command = pause ? BA_COMMAND_PCM_PAUSE : BA_COMMAND_PCM_RESUME;
	req.addr = transport->addr;
	req.type = transport->type;
	req.stream = transport->stream;

	return bluealsa_send_request(fd, &req);
}

/**
 * Drain PCM transport.
 *
 * @param fd Opened socket file descriptor.
 * @param transport Address to the transport structure with the addr, type
 *   and stream fields set - other fields are not used by this function.
 * @return Upon success this function returns 0. Otherwise, -1 is returned. */
int bluealsa_drain_transport(int fd, const struct ba_msg_transport *transport) {
	struct ba_request req;

#ifdef DEBUG
	char addr_[18];
	ba2str_(&req.addr, addr_);
	debug("Requesting PCM drain for %s\n", addr_);
#endif

	req.command = BA_COMMAND_PCM_DRAIN;
	req.addr = transport->addr;
	req.type = transport->type;
	req.stream = transport->stream;

	return bluealsa_send_request(fd, &req);
}

/**
 * Send RFCOMM message.
 *
 * @param fd Opened socket file descriptor.
 * @param addr MAC address of the Bluetooth device.
 * @param command NULL-terminated command string.
 * @return Upon success this function returns 0. Otherwise, -1 is returned. */
int bluealsa_send_rfcomm_command(int fd, bdaddr_t addr, const char *command) {
	struct ba_request req;

	/* snprintf() guarantees terminating NULL character */
	snprintf(req.rfcomm_command, sizeof(req.rfcomm_command), "%s", command);

#ifdef DEBUG
	char addr_[18];
	ba2str_(&req.addr, addr_);
	debug("Sending RFCOMM command to %s: %s\n", addr_, req.rfcomm_command);
#endif

	req.command = BA_COMMAND_RFCOMM_SEND;
	req.addr = addr;

	return bluealsa_send_request(fd, &req);
}
