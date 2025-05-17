#ifndef USB_MONITOR_H
#define USB_MONITOR_H

void list_usb_devices(const char *mount_point);
void save_usb_snapshot(const char *path, const char *output_file);
void compare_usb_snapshot(const char *path, const char *snapshot_file);

#endif
