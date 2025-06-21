#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#define CONFIG_USB_PATH_FORMAT "/home/%s/.matcomguard.conf"
#define CONFIG_PROC_PATH_FORMAT "/home/%s/.matcomguard_proc.conf"

static GtkWidget *combo_usb;
static GtkTextBuffer *output_buffer;
static GtkTextBuffer *usb_auto_buffer;

double alert_threshold = 10.0;
double proc_cpu_threshold = 70.0;
double proc_ram_threshold = 50.0;

void save_alert_threshold(double threshold) {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_USB_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%.1f\n", threshold);
        fclose(f);
    }
}

void load_alert_threshold() {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_USB_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "r");
    if (f) {
        fscanf(f, "%lf", &alert_threshold);
        fclose(f);
    }
}

void save_proc_thresholds(double cpu, double ram) {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_PROC_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%.1f %.1f\n", cpu, ram);
        fclose(f);
    }
}

void load_proc_thresholds() {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_PROC_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "r");
    if (f) {
        fscanf(f, "%lf %lf", &proc_cpu_threshold, &proc_ram_threshold);
        fclose(f);
    }
}

static void scan_usb(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);
    const gchar *selected_usb = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_usb));

    if (!selected_usb || strlen(selected_usb) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                   "No se seleccionó ningún dispositivo USB.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./build/matcom_guard \"%s\" %.1f", selected_usb, alert_threshold);

    FILE *fp = popen(cmd, "r");
    if (!fp) return;

    char line[1024];
    GtkTextIter iter;
    gtk_text_buffer_set_text(buffer, "", -1);
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

    while (fgets(line, sizeof(line), fp)) {
        gtk_text_buffer_insert(buffer, &iter, line, -1);
    }

    pclose(fp);
}

static void monitor_processes(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./build/matcom_guard_proc %.1f %.1f", proc_cpu_threshold, proc_ram_threshold);

    FILE *fp = popen(cmd, "r");
    if (!fp) return;

    char line[1024];
    GtkTextIter iter;
    gtk_text_buffer_set_text(buffer, "", -1);
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

    while (fgets(line, sizeof(line), fp)) {
        gtk_text_buffer_insert(buffer, &iter, line, -1);
    }

    pclose(fp);
}

static void scan_ports(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(data);
    char cmd[] = "./build/matcom_guard_ports";

    FILE *fp = popen(cmd, "r");
    if (!fp) return;

    char line[1024];
    GtkTextIter iter;
    gtk_text_buffer_set_text(buffer, "", -1);
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

    while (fgets(line, sizeof(line), fp)) {
        gtk_text_buffer_insert(buffer, &iter, line, -1);
    }

    pclose(fp);
}

static void open_settings_dialog(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Configuración de Umbral USB",
                                                    NULL, GTK_DIALOG_MODAL,
                                                    "Aceptar", GTK_RESPONSE_OK,
                                                    "Cancelar", GTK_RESPONSE_CANCEL,
                                                    NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *spin = gtk_spin_button_new_with_range(1.0, 100.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), alert_threshold);

    GtkWidget *label = gtk_label_new("Umbral de alerta USB (%):");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), spin, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), box);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        alert_threshold = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
        save_alert_threshold(alert_threshold);
    }

    gtk_widget_destroy(dialog);
}

static void open_proc_settings_dialog(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Configuración de Umbrales de Procesos",
                                                    NULL, GTK_DIALOG_MODAL,
                                                    "Aceptar", GTK_RESPONSE_OK,
                                                    "Cancelar", GTK_RESPONSE_CANCEL,
                                                    NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *spin_cpu = gtk_spin_button_new_with_range(1.0, 100.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_cpu), proc_cpu_threshold);

    GtkWidget *spin_ram = gtk_spin_button_new_with_range(1.0, 100.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_ram), proc_ram_threshold);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *label_cpu = gtk_label_new("Umbral de CPU (%):");
    GtkWidget *row_cpu = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(row_cpu), label_cpu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_cpu), spin_cpu, FALSE, FALSE, 0);

    GtkWidget *label_ram = gtk_label_new("Umbral de RAM (%):");
    GtkWidget *row_ram = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(row_ram), label_ram, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_ram), spin_ram, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), row_cpu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row_ram, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), box);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        proc_cpu_threshold = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_cpu));
        proc_ram_threshold = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_ram));
        save_proc_thresholds(proc_cpu_threshold, proc_ram_threshold);
    }

    gtk_widget_destroy(dialog);
}

void *usb_auto_monitor_thread(void *arg) {
    GtkTextBuffer *buffer = (GtkTextBuffer *)arg;
    while (1) {
        FILE *fp = popen("./build/matcom_guard_auto", "r");
        if (fp) {
            char line[1024];
            GtkTextIter iter;
            gtk_text_buffer_set_text(buffer, "", -1);
            gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
            while (fgets(line, sizeof(line), fp)) {
                gtk_text_buffer_insert(buffer, &iter, line, -1);
            }
            pclose(fp);
        }
        sleep(60);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    load_alert_threshold();
    load_proc_thresholds();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MatCom Guard - El Gran Salón del Trono");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 480);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    GtkWidget *label = gtk_label_new("Seleccione una acción de defensa:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 3, 1);

    GtkWidget *usb_label = gtk_label_new("Memoria USB:");
    gtk_grid_attach(GTK_GRID(grid), usb_label, 0, 1, 1, 1);

    combo_usb = gtk_combo_box_text_new();
    char base_path[256];
    snprintf(base_path, sizeof(base_path), "/media/%s", getenv("USER"));
    DIR *dir = opendir(base_path);
    struct dirent *entry;
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR &&
                strcmp(entry->d_name, ".") != 0 &&
                strcmp(entry->d_name, "..") != 0) {
                gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_usb), entry->d_name);
            }
        }
        closedir(dir);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_usb), 0);
    gtk_grid_attach(GTK_GRID(grid), combo_usb, 1, 1, 1, 1);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_grid_attach(GTK_GRID(grid), scroll, 0, 7, 2, 1);

    // Segunda área de texto para el monitoreo automático
    GtkWidget *usb_auto_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(usb_auto_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(usb_auto_view), GTK_WRAP_WORD);
    usb_auto_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(usb_auto_view));
    GtkWidget *auto_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(auto_scroll), usb_auto_view);
    gtk_widget_set_vexpand(auto_scroll, TRUE);
    gtk_grid_attach(GTK_GRID(grid), auto_scroll, 2, 7, 2, 1);

    GtkWidget *btn_usb = gtk_button_new_with_label("2. Escanear memoria USB");
    g_signal_connect(btn_usb, "clicked", G_CALLBACK(scan_usb), output_buffer);
    gtk_grid_attach(GTK_GRID(grid), btn_usb, 0, 2, 2, 1);

    GtkWidget *btn_usb_config = gtk_button_new_with_label("⚙ Ajustes de Umbral USB");
    g_signal_connect(btn_usb_config, "clicked", G_CALLBACK(open_settings_dialog), NULL);
    gtk_grid_attach(GTK_GRID(grid), btn_usb_config, 0, 3, 2, 1);

    GtkWidget *btn_proc = gtk_button_new_with_label("3. Monitorear procesos");
    g_signal_connect(btn_proc, "clicked", G_CALLBACK(monitor_processes), output_buffer);
    gtk_grid_attach(GTK_GRID(grid), btn_proc, 0, 4, 2, 1);

    GtkWidget *btn_proc_config = gtk_button_new_with_label("⚙ Ajustes de Umbral de Procesos");
    g_signal_connect(btn_proc_config, "clicked", G_CALLBACK(open_proc_settings_dialog), NULL);
    gtk_grid_attach(GTK_GRID(grid), btn_proc_config, 0, 5, 2, 1);

    GtkWidget *btn_ports = gtk_button_new_with_label("4. Escanear puertos locales");
    g_signal_connect(btn_ports, "clicked", G_CALLBACK(scan_ports), output_buffer);
    gtk_grid_attach(GTK_GRID(grid), btn_ports, 0, 6, 2, 1);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);

    // Iniciar hilo para escaneo automático de USB
    pthread_t auto_thread;
    pthread_create(&auto_thread, NULL, usb_auto_monitor_thread, usb_auto_buffer);

    gtk_main();
    return 0;
}
