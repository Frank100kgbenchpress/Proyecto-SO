#ifndef USB_MONITOR_H
#define USB_MONITOR_H

void list_usb_devices(const char *mount_point);

void compute_sha256(const char *filepath, char *output, size_t maxlen);

void save_usb_security_baseline(const char *usb_path, const char *baseline_file);

void compare_usb_security_baseline(const char *usb_path, const char *baseline_file, double alert_threshold_percent);

#endif
