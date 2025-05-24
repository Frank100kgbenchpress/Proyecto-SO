#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

static GtkWidget *combo_usb;

static void scan_usb(GtkWidget *widget, gpointer data) {
    const gchar *selected_usb = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_usb));

    if (!selected_usb || strlen(selected_usb) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                   "No se seleccionó ningún dispositivo USB.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./build/matcom_guard \"%s\"", selected_usb);

    printf("DEBUG: Ejecutando → %s\n", cmd);
    system(cmd);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MatCom Guard - El Gran Salón del Trono");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    GtkWidget *label = gtk_label_new("Seleccione una acción de defensa:");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);

    GtkWidget *usb_label = gtk_label_new("Memoria USB:");
    gtk_grid_attach(GTK_GRID(grid), usb_label, 0, 1, 1, 1);

    combo_usb = gtk_combo_box_text_new();

    // Llenar combo con dispositivos en /media/$USER
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

    GtkWidget *btn_usb = gtk_button_new_with_label("2. Escanear memoria USB");
    g_signal_connect(btn_usb, "clicked", G_CALLBACK(scan_usb), NULL);
    gtk_grid_attach(GTK_GRID(grid), btn_usb, 0, 2, 2, 1);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
