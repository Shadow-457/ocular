#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

int num_images = 0;
char **image_paths = NULL;
int current_index = 0;

GtkWidget *drawing_area;
GtkWidget *window;
GdkPixbuf *current_pixbuf = NULL;
gboolean slideshow_active = FALSE;
guint slideshow_timeout_id = 0;

gboolean draw_image_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
    // Fill the background
    cairo_set_source_rgb(cr, 0.07, 0.07, 0.07); // #121212
    cairo_paint(cr);

    if (!current_pixbuf) return FALSE;

    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);
    int pw = gdk_pixbuf_get_width(current_pixbuf);
    int ph = gdk_pixbuf_get_height(current_pixbuf);

    double scale_x = (double)w / pw;
    double scale_y = (double)h / ph;
    double scale = MIN(scale_x, scale_y);

    int new_w = (int)(pw * scale);
    int new_h = (int)(ph * scale);

    double offset_x = (w - new_w) / 2.0;
    double offset_y = (h - new_h) / 2.0;

    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);

    gdk_cairo_set_source_pixbuf(cr, current_pixbuf, 0, 0);
    cairo_paint(cr);

    return FALSE;
}

void load_image(const char *path) {
    if (current_pixbuf) {
        g_object_unref(current_pixbuf);
        current_pixbuf = NULL;
    }
    GError *error = NULL;
    current_pixbuf = gdk_pixbuf_new_from_file(path, &error);
    if (error) {
        g_printerr("Error loading %.100s: %s\n", path, error->message);
        g_error_free(error);
        return;
    }
    const char* filename = g_strrstr(path, "/");
    filename = filename ? filename + 1 : path;
    char title[1024];
    snprintf(title, sizeof(title), "Ocular - %s", filename);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_widget_queue_draw(drawing_area);
}

void next_image(GtkWidget *w, gpointer data) {
    if (num_images > 0) {
        current_index = (current_index + 1) % num_images;
        load_image(image_paths[current_index]);
    }
}

void prev_image(GtkWidget *w, gpointer data) {
    if (num_images > 0) {
        current_index = (current_index - 1 + num_images) % num_images;
        load_image(image_paths[current_index]);
    }
}

gboolean slideshow_step(gpointer data) {
    next_image(NULL, NULL);
    return G_SOURCE_CONTINUE;
}

void toggle_slideshow(GtkButton *button, gpointer data) {
    slideshow_active = !slideshow_active;
    if (slideshow_active) {
        gtk_button_set_label(button, "⏸");
        slideshow_timeout_id = g_timeout_add(3000, slideshow_step, NULL); // 3 seconds
    } else {
        gtk_button_set_label(button, "▶");
        if (slideshow_timeout_id > 0) {
            g_source_remove(slideshow_timeout_id);
            slideshow_timeout_id = 0;
        }
    }
}

gboolean key_press_cb(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_space || event->keyval == GDK_KEY_j) {
        next_image(NULL, NULL);
        return TRUE;
    } else if (event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_k || event->keyval == GDK_KEY_BackSpace) {
        prev_image(NULL, NULL);
        return TRUE;
    } else if (event->keyval == GDK_KEY_q || event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    } else if (event->keyval == GDK_KEY_f || event->keyval == GDK_KEY_F11) {
        static gboolean fullscreen = FALSE;
        fullscreen = !fullscreen;
        if (fullscreen) {
            gtk_window_fullscreen(GTK_WINDOW(window));
        } else {
            gtk_window_unfullscreen(GTK_WINDOW(window));
        }
        return TRUE;
    }
    return FALSE;
}

int is_image_file(const char *ext) {
    if (!ext) return 0;
    return (g_ascii_strcasecmp(ext, "jpg") == 0 || g_ascii_strcasecmp(ext, "jpeg") == 0 ||
            g_ascii_strcasecmp(ext, "png") == 0 || g_ascii_strcasecmp(ext, "gif") == 0 ||
            g_ascii_strcasecmp(ext, "bmp") == 0 || g_ascii_strcasecmp(ext, "webp") == 0 ||
            g_ascii_strcasecmp(ext, "svg") == 0);
}

int compare_strings(const void *a, const void *b) {
    return g_ascii_strcasecmp(*(const char**)a, *(const char**)b);
}

void load_directory(const char *path, const char *initial_file) {
    DIR *dir = opendir(path);
    if (!dir) return;

    int capacity = 10;
    image_paths = malloc(capacity * sizeof(char*));
    
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG || ent->d_type == DT_LNK || ent->d_type == DT_UNKNOWN) {
            const char *ext = g_strrstr(ent->d_name, ".");
            if (ext) {
                if (is_image_file(ext + 1)) {
                    if (num_images >= capacity) {
                        capacity *= 2;
                        image_paths = realloc(image_paths, capacity * sizeof(char*));
                    }
                    image_paths[num_images++] = g_build_filename(path, ent->d_name, NULL);
                }
            }
        }
    }
    closedir(dir);

    if (num_images > 0) {
        qsort(image_paths, num_images, sizeof(char*), compare_strings);
        if (initial_file) {
            for (int i = 0; i < num_images; ++i) {
                if (strcmp(image_paths[i], initial_file) == 0) {
                    current_index = i;
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Apply native dark theme for a better viewing experience
    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", TRUE, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Ocular");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(key_press_cb), NULL);
    
    // Set SVG icon
    gtk_window_set_icon_name(GTK_WINDOW(window), "ocular");
    const char *icon_path_local = "ocular.svg";
    const char *icon_path_sys = "/usr/share/icons/hicolor/scalable/apps/ocular.svg";
    if (g_file_test(icon_path_local, G_FILE_TEST_EXISTS)) {
        gtk_window_set_icon_from_file(GTK_WINDOW(window), icon_path_local, NULL);
    } else if (g_file_test(icon_path_sys, G_FILE_TEST_EXISTS)) {
        gtk_window_set_icon_from_file(GTK_WINDOW(window), icon_path_sys, NULL);
    }

    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);

    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    gtk_container_add(GTK_CONTAINER(overlay), drawing_area);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(draw_image_cb), NULL);

    // Top-left compact slideshow button
    GtkWidget *slideshow_btn = gtk_button_new_with_label("▶");
    gtk_button_set_relief(GTK_BUTTON(slideshow_btn), GTK_RELIEF_NONE);
    gtk_widget_set_halign(slideshow_btn, GTK_ALIGN_START);
    gtk_widget_set_valign(slideshow_btn, GTK_ALIGN_START);
    gtk_widget_set_margin_start(slideshow_btn, 10);
    gtk_widget_set_margin_top(slideshow_btn, 10);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), slideshow_btn);
    g_signal_connect(slideshow_btn, "clicked", G_CALLBACK(toggle_slideshow), NULL);

    // Provide a neat CSS style for larger semi-transparent navigation buttons
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "button.nav-btn { font-size: 24px; font-weight: bold; padding: 10px 20px; color: rgba(255,255,255,0.7); } button.nav-btn:hover { color: rgba(255,255,255,1); }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    // Left previous button
    GtkWidget *prev_btn = gtk_button_new_with_label("❮");
    gtk_button_set_relief(GTK_BUTTON(prev_btn), GTK_RELIEF_NONE);
    gtk_widget_set_halign(prev_btn, GTK_ALIGN_START);
    gtk_widget_set_valign(prev_btn, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(prev_btn), "nav-btn");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), prev_btn);
    g_signal_connect(prev_btn, "clicked", G_CALLBACK(prev_image), NULL);

    // Right next button
    GtkWidget *next_btn = gtk_button_new_with_label("❯");
    gtk_button_set_relief(GTK_BUTTON(next_btn), GTK_RELIEF_NONE);
    gtk_widget_set_halign(next_btn, GTK_ALIGN_END);
    gtk_widget_set_valign(next_btn, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(next_btn), "nav-btn");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), next_btn);
    g_signal_connect(next_btn, "clicked", G_CALLBACK(next_image), NULL);

    const char *target_path = ".";
    if (argc > 1) {
        target_path = argv[1];
    }
    
    char *abs_path = realpath(target_path, NULL);
    if (abs_path) {
        if (g_file_test(abs_path, G_FILE_TEST_IS_DIR)) {
            load_directory(abs_path, NULL);
        } else if (g_file_test(abs_path, G_FILE_TEST_IS_REGULAR)) {
            char *dir = g_path_get_dirname(abs_path);
            load_directory(dir, abs_path);
            g_free(dir);
        }
        g_free(abs_path);
    } else {
        load_directory(".", NULL);
    }

    if (num_images > 0) {
        load_image(image_paths[current_index]);
    } else {
        g_print("No images found.\n");
    }

    gtk_widget_show_all(window);
    gtk_main();

    for (int i = 0; i < num_images; i++) {
        g_free(image_paths[i]);
    }
    free(image_paths);
    if (current_pixbuf) {
        g_object_unref(current_pixbuf);
    }

    return 0;
}
