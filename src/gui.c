#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#define CONFIG_USB_PATH_FORMAT  "/home/%s/.matcomguard.conf"
#define CONFIG_PROC_PATH_FORMAT "/home/%s/.matcomguard_proc.conf"

static GtkWidget      *combo_usb;
static GtkTextBuffer  *output_buffer;
static GtkTextBuffer  *usb_auto_buffer;
static GtkTextBuffer  *proc_auto_buffer;

double alert_threshold     = 10.0;
double proc_cpu_threshold  = 70.0;
double proc_ram_threshold  = 50.0;

/* Estructura para pasar datos a push_text_to_buffer() */
typedef struct {
    GtkTextBuffer *buffer;
    char          *text;
} UpdateData;

/* Callback que corre en el hilo principal GTK y escribe en el buffer */
static gboolean
push_text_to_buffer(gpointer user_data) {
    UpdateData *ud = user_data;
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(ud->buffer, &iter);
    gtk_text_buffer_insert(ud->buffer, &iter, ud->text, -1);
    g_free(ud->text);
    g_free(ud);
    return FALSE;
}

void save_alert_threshold(double threshold) {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_USB_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%.1f\n", threshold); fclose(f); }
}
void load_alert_threshold() {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_USB_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "r");
    if (f) { fscanf(f, "%lf", &alert_threshold); fclose(f); }
}
void save_proc_thresholds(double cpu, double ram) {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_PROC_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "w");
    if (f) { fprintf(f, "%.1f %.1f\n", cpu, ram); fclose(f); }
}
void load_proc_thresholds() {
    char path[256];
    snprintf(path, sizeof(path), CONFIG_PROC_PATH_FORMAT, getenv("USER"));
    FILE *f = fopen(path, "r");
    if (f) { fscanf(f, "%lf %lf", &proc_cpu_threshold, &proc_ram_threshold); fclose(f); }
}

static void scan_usb(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = data;
    const gchar *usb = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_usb));
    if (!usb) {
        GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "No se seleccionó ningún USB.");
        gtk_dialog_run(GTK_DIALOG(d)); gtk_widget_destroy(d);
        return;
    }
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./build/matcom_guard \"%s\" %.1f", usb, alert_threshold);
    FILE *fp = popen(cmd, "r");
    if (!fp) return;
    gtk_text_buffer_set_text(buffer, "", -1);
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        UpdateData *ud = g_new0(UpdateData,1);
        ud->buffer = buffer;
        ud->text   = g_strdup(line);
        g_idle_add(push_text_to_buffer, ud);
    }
    pclose(fp);
}

static void monitor_processes(GtkWidget *widget, gpointer data) {
    GtkTextBuffer *buffer = data;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./build/matcom_guard_proc %.1f %.1f",
             proc_cpu_threshold, proc_ram_threshold);
    FILE *fp = popen(cmd, "r");
    if (!fp) return;
    gtk_text_buffer_set_text(buffer, "", -1);
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        UpdateData *ud = g_new0(UpdateData,1);
        ud->buffer = buffer;
        ud->text   = g_strdup(line);
        g_idle_add(push_text_to_buffer, ud);
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
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
      "Umbral USB", NULL, GTK_DIALOG_MODAL,
      "Aceptar", GTK_RESPONSE_OK, "Cancelar", GTK_RESPONSE_CANCEL, NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *spin = gtk_spin_button_new_with_range(1.0,100.0,1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), alert_threshold);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Umbral USB (%):"), FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(box), spin, FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(area), box);
    gtk_widget_show_all(dlg);
    if (gtk_dialog_run(GTK_DIALOG(dlg))==GTK_RESPONSE_OK) {
        alert_threshold = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
        save_alert_threshold(alert_threshold);
    }
    gtk_widget_destroy(dlg);
}

static void open_proc_settings_dialog(GtkWidget *widget, gpointer data) {
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
      "Umbral Procesos", NULL, GTK_DIALOG_MODAL,
      "Aceptar", GTK_RESPONSE_OK, "Cancelar", GTK_RESPONSE_CANCEL, NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *spin_cpu = gtk_spin_button_new_with_range(1.0,100.0,1.0);
    GtkWidget *spin_ram = gtk_spin_button_new_with_range(1.0,100.0,1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_cpu), proc_cpu_threshold);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_ram), proc_ram_threshold);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,10);
    GtkWidget *h1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10),
              *h2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
    gtk_box_pack_start(GTK_BOX(h1), gtk_label_new("CPU (%):"), FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(h1), spin_cpu, FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(h2), gtk_label_new("RAM (%):"), FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(h2), spin_ram, FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox), h1, FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(vbox), h2, FALSE,FALSE,0);
    gtk_container_add(GTK_CONTAINER(area), vbox);
    gtk_widget_show_all(dlg);
    if (gtk_dialog_run(GTK_DIALOG(dlg))==GTK_RESPONSE_OK) {
        proc_cpu_threshold = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_cpu));
        proc_ram_threshold = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_ram));
        save_proc_thresholds(proc_cpu_threshold, proc_ram_threshold);
    }
    gtk_widget_destroy(dlg);
}

/* Hilo de monitoreo automático USB */
void *usb_auto_monitor_thread(void *arg) {
    GtkTextBuffer *buffer = arg;
    while (1) {
        FILE *fp = popen("./build/matcom_guard_auto", "r");
        if (fp) {
            char line[1024];
            /* limpiar */
            UpdateData *clr = g_new0(UpdateData,1);
            clr->buffer = buffer; clr->text = g_strdup("");
            g_idle_add(push_text_to_buffer, clr);
            /* luego cada línea */
            while (fgets(line, sizeof(line), fp)) {
                UpdateData *ud = g_new0(UpdateData,1);
                ud->buffer = buffer;
                ud->text   = g_strdup(line);
                g_idle_add(push_text_to_buffer, ud);
            }
            pclose(fp);
        }
        sleep(60);
    }
    return NULL;
}

/* Hilo de monitoreo automático procesos */
void *proc_auto_monitor_thread(void *arg) {
    GtkTextBuffer *buffer = arg;
    while (1) {
        FILE *fp = popen("./build/matcom_guard_proc_auto", "r");
        if (fp) {
            char line[1024];
            UpdateData *clr = g_new0(UpdateData,1);
            clr->buffer = buffer; clr->text = g_strdup("");
            g_idle_add(push_text_to_buffer, clr);
            while (fgets(line, sizeof(line), fp)) {
                UpdateData *ud = g_new0(UpdateData,1);
                ud->buffer = buffer;
                ud->text   = g_strdup(line);
                g_idle_add(push_text_to_buffer, ud);
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
    gtk_grid_set_row_spacing(GTK_GRID(grid),    10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    gtk_grid_attach(GTK_GRID(grid),
        gtk_label_new("Seleccione una acción de defensa:"), 0, 0, 3, 1);

    /* Combo USB */
    gtk_grid_attach(GTK_GRID(grid),
        gtk_label_new("Memoria USB:"), 0,1,1,1);
    combo_usb = gtk_combo_box_text_new();
    {
        char path[256];
        snprintf(path, sizeof(path), "/media/%s", getenv("USER"));
        DIR *d = opendir(path);
        struct dirent *e;
        if (d) {
            while ((e=readdir(d))) {
                if (e->d_type==DT_DIR && strcmp(e->d_name,".") && strcmp(e->d_name,".."))
                    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_usb), e->d_name);
            }
            closedir(d);
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_usb), 0);
    gtk_grid_attach(GTK_GRID(grid), combo_usb, 1,1,1,1);

    /* Área principal de texto */
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD);
    output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkWidget *sc = gtk_scrolled_window_new(NULL,NULL);
    gtk_container_add(GTK_CONTAINER(sc), tv);
    gtk_widget_set_vexpand(sc, TRUE);
    gtk_grid_attach(GTK_GRID(grid), sc, 0,7,2,1);

    /* Área automática USB */
    GtkWidget *tv2 = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv2), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv2), GTK_WRAP_WORD);
    usb_auto_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv2));
    GtkWidget *sc2 = gtk_scrolled_window_new(NULL,NULL);
    gtk_container_add(GTK_CONTAINER(sc2), tv2);
    gtk_widget_set_vexpand(sc2, TRUE);
    gtk_grid_attach(GTK_GRID(grid), sc2, 2,7,16,1);

    /* Área automática procesos */
    GtkWidget *tv3 = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv3), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv3), GTK_WRAP_WORD);
    proc_auto_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv3));
    GtkWidget *sc3 = gtk_scrolled_window_new(NULL,NULL);
    gtk_container_add(GTK_CONTAINER(sc3), tv3);
    gtk_widget_set_vexpand(sc3, TRUE);
    gtk_grid_attach(GTK_GRID(grid), sc3, 20,7,16,1);

    /* Botones */
    GtkWidget *b1 = gtk_button_new_with_label("2. Escanear USB");
    g_signal_connect(b1,"clicked",G_CALLBACK(scan_usb),output_buffer);
    gtk_grid_attach(GTK_GRID(grid),b1,0,2,2,1);

    GtkWidget *b2 = gtk_button_new_with_label("⚙ Umbral USB");
    g_signal_connect(b2,"clicked",G_CALLBACK(open_settings_dialog),NULL);
    gtk_grid_attach(GTK_GRID(grid),b2,0,3,2,1);

    GtkWidget *b3 = gtk_button_new_with_label("3. Monitorear Proc");
    g_signal_connect(b3,"clicked",G_CALLBACK(monitor_processes),output_buffer);
    gtk_grid_attach(GTK_GRID(grid),b3,0,4,2,1);

    GtkWidget *b4 = gtk_button_new_with_label("⚙ Umbral Proc");
    g_signal_connect(b4,"clicked",G_CALLBACK(open_proc_settings_dialog),NULL);
    gtk_grid_attach(GTK_GRID(grid),b4,0,5,2,1);

    GtkWidget *b5 = gtk_button_new_with_label("4. Escanear Puertos");
    g_signal_connect(b5,"clicked",G_CALLBACK(scan_ports),output_buffer);
    gtk_grid_attach(GTK_GRID(grid),b5,0,6,2,1);

    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    gtk_widget_show_all(window);

    /* Lanzar hilos de auto-monitoreo */
    pthread_t t1, t2;
    pthread_create(&t1, NULL, usb_auto_monitor_thread, usb_auto_buffer);
    pthread_create(&t2, NULL, proc_auto_monitor_thread, proc_auto_buffer);

    gtk_main();
    return 0;
}
