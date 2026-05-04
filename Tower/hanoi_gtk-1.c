/* ============================================================
 *  Towers of Hanoi — GTK3 Graphical Game
 *
 *  Features:
 *    • Animated pegs drawn with Cairo
 *    • Coloured disks (gradient fills)
 *    • Step-by-step solver (Next / Auto-Play / Reset)
 *    • Speed slider for Auto-Play
 *    • Info bar: step counter, move description
 *    • About dialog
 *
 *  Compile:
 *    gcc $(pkg-config --cflags gtk+-3.0) -o hanoi_gtk hanoi_gtk.c \
 *        $(pkg-config --libs gtk+-3.0) -lm -Wall
 *
 *  Run:
 *    ./hanoi_gtk
 * ============================================================ */

#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────────────────────
 *  Game constants
 * ────────────────────────────────────────────── */
#define MAX_DISKS     8
#define NUM_PEGS      3
#define CANVAS_W      750
#define CANVAS_H      420
#define PEG_BASE_Y    370           /* y of the base line               */
#define PEG_TOP_Y      60           /* y of the peg top                 */
#define PEG_WIDTH       8
#define DISK_H         22           /* height of every disk             */
#define DISK_MIN_W     30           /* smallest disk pixel width        */
#define DISK_STEP      20           /* width increment per disk size    */
#define BASE_H         14           /* base platform height             */

/* Peg centre x positions */
static const int PEG_X[NUM_PEGS] = { 125, 375, 625 };

/* Disk palette  (r, g, b) — up to 8 disks */
static const double DISK_COLOR[MAX_DISKS][3] = {
    {0.95, 0.20, 0.20},   /* 1 — red           */
    {0.95, 0.55, 0.10},   /* 2 — orange        */
    {0.90, 0.85, 0.10},   /* 3 — yellow        */
    {0.20, 0.78, 0.25},   /* 4 — green         */
    {0.15, 0.60, 0.90},   /* 5 — sky blue      */
    {0.25, 0.20, 0.85},   /* 6 — indigo        */
    {0.65, 0.15, 0.85},   /* 7 — violet        */
    {0.85, 0.20, 0.65},   /* 8 — pink          */
};

/* ──────────────────────────────────────────────
 *  Move list (built before animation starts)
 * ────────────────────────────────────────────── */
typedef struct {
    int src;
    int dst;
    int disk_size;
} Move;

#define MAX_MOVES  (1 << MAX_DISKS)   /* 255 for 8 disks */

static Move  move_list[MAX_MOVES];
static int   move_count   = 0;
static int   current_step = 0;        /* 0 = initial state shown       */

/* ──────────────────────────────────────────────
 *  Peg state
 * ────────────────────────────────────────────── */
static int pegs[NUM_PEGS][MAX_DISKS + 1]; /* [p][0]=count, [p][1..]=disk */
static int num_disks = 3;
static int from_peg  = 0;   /* 0-indexed */
static int to_peg    = 2;
static int temp_peg  = 1;

/* ──────────────────────────────────────────────
 *  GTK widgets (global so callbacks can reach)
 * ────────────────────────────────────────────── */
static GtkWidget *canvas;
static GtkWidget *lbl_step;
static GtkWidget *lbl_move;
static GtkWidget *btn_next;
static GtkWidget *btn_prev;
static GtkWidget *btn_auto;
static GtkWidget *btn_reset;
static GtkWidget *spin_disks;
static GtkWidget *combo_from;
static GtkWidget *combo_to;
static GtkWidget *speed_scale;

static guint      auto_timer_id = 0;
static gboolean   auto_running  = FALSE;

/* ──────────────────────────────────────────────
 *  Forward declarations
 * ────────────────────────────────────────────── */
static void build_move_list(int n, int src, int dst, int tmp);
static void apply_move(int idx);
static void undo_move(int idx);
static void reset_game(void);
static void update_ui(void);
static gboolean on_draw(GtkWidget *w, cairo_t *cr, gpointer data);
static void on_next(GtkWidget *w, gpointer data);
static void on_prev(GtkWidget *w, gpointer data);
static void on_auto(GtkWidget *w, gpointer data);
static void on_reset(GtkWidget *w, gpointer data);
static gboolean auto_step(gpointer data);

/* ============================================================
 *  Move list builder (recursive, mirrors hanoi logic)
 * ============================================================ */
static void build_move_list(int n, int src, int dst, int tmp)
{
    if (n == 0) return;
    build_move_list(n - 1, src, tmp, dst);
    /* Record this move */
    int top = pegs[src][0];
    move_list[move_count].src       = src;
    move_list[move_count].dst       = dst;
    move_list[move_count].disk_size = pegs[src][top];
    move_count++;
    /* Actually move it so subsequent levels are correct */
    int disk = pegs[src][top];
    pegs[src][top] = 0;
    pegs[src][0]--;
    pegs[dst][0]++;
    pegs[dst][ pegs[dst][0] ] = disk;
    build_move_list(n - 1, tmp, dst, src);
}

/* ──────────────────────────────────────────────
 *  Apply / undo a move by index in move_list
 * ────────────────────────────────────────────── */
static void apply_move(int idx)
{
    int src  = move_list[idx].src;
    int dst  = move_list[idx].dst;
    int top  = pegs[src][0];
    int disk = pegs[src][top];
    pegs[src][top] = 0;
    pegs[src][0]--;
    pegs[dst][0]++;
    pegs[dst][ pegs[dst][0] ] = disk;
}

static void undo_move(int idx)
{
    /* Reverse src/dst */
    int src  = move_list[idx].dst;
    int dst  = move_list[idx].src;
    int top  = pegs[src][0];
    int disk = pegs[src][top];
    pegs[src][top] = 0;
    pegs[src][0]--;
    pegs[dst][0]++;
    pegs[dst][ pegs[dst][0] ] = disk;
}

/* ──────────────────────────────────────────────
 *  Reset to initial state
 * ────────────────────────────────────────────── */
static void reset_game(void)
{
    int p, i;

    num_disks = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_disks));
    from_peg  = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_from));
    to_peg    = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_to));
    if (from_peg == to_peg) to_peg = (from_peg + 2) % 3;
    temp_peg  = 3 - from_peg - to_peg;   /* 0+1+2 = 3 */

    /* Clear pegs */
    for (p = 0; p < NUM_PEGS; p++)
        for (i = 0; i <= MAX_DISKS; i++)
            pegs[p][i] = 0;

    /* Stack all disks on from_peg, largest at bottom */
    pegs[from_peg][0] = num_disks;
    for (i = 0; i < num_disks; i++)
        pegs[from_peg][i + 1] = num_disks - i;

    /* Build full move list (this also simulates moves, so do it on
       a scratch copy of pegs — we re-init afterwards)            */
    move_count   = 0;
    current_step = 0;
    build_move_list(num_disks, from_peg, to_peg, temp_peg);

    /* Re-init pegs to initial state for display */
    for (p = 0; p < NUM_PEGS; p++)
        for (i = 0; i <= MAX_DISKS; i++)
            pegs[p][i] = 0;
    pegs[from_peg][0] = num_disks;
    for (i = 0; i < num_disks; i++)
        pegs[from_peg][i + 1] = num_disks - i;
}

/* ──────────────────────────────────────────────
 *  Update labels & button sensitivity
 * ────────────────────────────────────────────── */
static void update_ui(void)
{
    char buf[128];
    int  total = move_count;

    snprintf(buf, sizeof(buf),
             "Step  <b>%d</b> / %d", current_step, total);
    gtk_label_set_markup(GTK_LABEL(lbl_step), buf);

    if (current_step == 0) {
        gtk_label_set_text(GTK_LABEL(lbl_move), "Initial state");
    } else {
        Move *m = &move_list[current_step - 1];
        snprintf(buf, sizeof(buf),
                 "Moved disk [size <b>%d</b>]  :  Peg <b>%d</b>  ➜  Peg <b>%d</b>",
                 m->disk_size, m->src + 1, m->dst + 1);
        gtk_label_set_markup(GTK_LABEL(lbl_move), buf);
    }

    gtk_widget_set_sensitive(btn_prev, current_step > 0);
    gtk_widget_set_sensitive(btn_next, current_step < total);

    /* When finished, stop auto-play */
    if (current_step >= total && auto_running) {
        g_source_remove(auto_timer_id);
        auto_timer_id = 0;
        auto_running  = FALSE;
        gtk_button_set_label(GTK_BUTTON(btn_auto), "▶  Auto Play");
    }
}

/* ============================================================
 *  Cairo drawing
 * ============================================================ */
static void draw_disk(cairo_t *cr, int peg_idx, int slot, int disk_size)
{
    /*  slot: 1 = bottom, num_disks = top  */
    int w   = DISK_MIN_W + (disk_size - 1) * DISK_STEP;
    int x   = PEG_X[peg_idx] - w / 2;
    int y   = PEG_BASE_Y - BASE_H - slot * DISK_H;
    int r   = 6;  /* corner radius */

    double dr = DISK_COLOR[disk_size - 1][0];
    double dg = DISK_COLOR[disk_size - 1][1];
    double db = DISK_COLOR[disk_size - 1][2];

    /* Rounded rectangle path */
    cairo_new_path(cr);
    cairo_arc(cr, x + r,     y + r,          r, M_PI,       3*M_PI/2);
    cairo_arc(cr, x + w - r, y + r,          r, 3*M_PI/2,   0);
    cairo_arc(cr, x + w - r, y + DISK_H - r, r, 0,          M_PI/2);
    cairo_arc(cr, x + r,     y + DISK_H - r, r, M_PI/2,     M_PI);
    cairo_close_path(cr);

    /* Gradient fill */
    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + DISK_H);
    cairo_pattern_add_color_stop_rgb(pat, 0.0, dr + 0.18, dg + 0.18, db + 0.18);
    cairo_pattern_add_color_stop_rgb(pat, 1.0, dr * 0.55, dg * 0.55, db * 0.55);
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);

    /* Border */
    cairo_set_source_rgb(cr, dr * 0.35, dg * 0.35, db * 0.35);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    /* Size label */
    char label[4];
    snprintf(label, sizeof(label), "%d", disk_size);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 11.0);
    cairo_text_extents_t te;
    cairo_text_extents(cr, label, &te);
    cairo_move_to(cr,
                  PEG_X[peg_idx] - te.width / 2 - te.x_bearing,
                  y + DISK_H / 2.0 - te.height / 2 - te.y_bearing);
    cairo_show_text(cr, label);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    (void)widget; (void)data;
    int p, s;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    double cw = alloc.width;
    double ch = alloc.height;
    double sx = cw / CANVAS_W;
    double sy = ch / CANVAS_H;
    cairo_scale(cr, sx, sy);

    /* ── Background gradient ── */
    cairo_pattern_t *bg = cairo_pattern_create_linear(0, 0, 0, CANVAS_H);
    cairo_pattern_add_color_stop_rgb(bg, 0.0, 0.08, 0.08, 0.18);
    cairo_pattern_add_color_stop_rgb(bg, 1.0, 0.02, 0.02, 0.08);
    cairo_set_source(cr, bg);
    cairo_rectangle(cr, 0, 0, CANVAS_W, CANVAS_H);
    cairo_fill(cr);
    cairo_pattern_destroy(bg);

    /* ── Title text ── */
    cairo_set_source_rgba(cr, 0.85, 0.85, 1.0, 0.35);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 28.0);
    cairo_move_to(cr, 170, 42);
    cairo_show_text(cr, "TOWERS  OF  HANOI");

    /* ── Peg labels ── */
    cairo_set_font_size(cr, 14.0);
    const char *pnames[3] = {"Peg 1", "Peg 2", "Peg 3"};
    for (p = 0; p < 3; p++) {
        cairo_text_extents_t te;
        cairo_text_extents(cr, pnames[p], &te);
        double lx = PEG_X[p] - te.width / 2 - te.x_bearing;

        /* Highlight from/to pegs */
        if (p == from_peg)
            cairo_set_source_rgba(cr, 0.40, 0.90, 0.40, 0.85);
        else if (p == to_peg)
            cairo_set_source_rgba(cr, 0.40, 0.65, 1.00, 0.85);
        else
            cairo_set_source_rgba(cr, 0.70, 0.70, 0.85, 0.60);

        cairo_move_to(cr, lx, PEG_BASE_Y + BASE_H + 18);
        cairo_show_text(cr, pnames[p]);
    }

    /* ── Base platforms ── */
    for (p = 0; p < 3; p++) {
        int bw = 180;
        int bx = PEG_X[p] - bw / 2;
        int by = PEG_BASE_Y;

        cairo_pattern_t *bp = cairo_pattern_create_linear(bx, by, bx, by + BASE_H);
        cairo_pattern_add_color_stop_rgb(bp, 0.0, 0.55, 0.45, 0.25);
        cairo_pattern_add_color_stop_rgb(bp, 1.0, 0.30, 0.22, 0.10);
        cairo_set_source(cr, bp);
        cairo_rectangle(cr, bx, by, bw, BASE_H);
        cairo_fill(cr);
        cairo_pattern_destroy(bp);

        cairo_set_source_rgba(cr, 0.80, 0.70, 0.40, 0.6);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bx, by, bw, BASE_H);
        cairo_stroke(cr);
    }

    /* ── Peg poles ── */
    for (p = 0; p < 3; p++) {
        cairo_pattern_t *pp =
            cairo_pattern_create_linear(PEG_X[p] - PEG_WIDTH/2, 0,
                                        PEG_X[p] + PEG_WIDTH/2, 0);
        cairo_pattern_add_color_stop_rgb(pp, 0.0, 0.75, 0.60, 0.30);
        cairo_pattern_add_color_stop_rgb(pp, 0.5, 0.95, 0.85, 0.55);
        cairo_pattern_add_color_stop_rgb(pp, 1.0, 0.50, 0.38, 0.15);
        cairo_set_source(cr, pp);
        cairo_rectangle(cr, PEG_X[p] - PEG_WIDTH/2,
                        PEG_TOP_Y,
                        PEG_WIDTH,
                        PEG_BASE_Y - PEG_TOP_Y);
        cairo_fill(cr);
        cairo_pattern_destroy(pp);
    }

    /* ── Disks ── */
    for (p = 0; p < NUM_PEGS; p++) {
        int count = pegs[p][0];
        for (s = 1; s <= count; s++)
            draw_disk(cr, p, s, pegs[p][s]);
    }

    /* ── "SOLVED!" banner ── */
    if (current_step == move_count && move_count > 0) {
        cairo_set_source_rgba(cr, 0.20, 0.90, 0.50, 0.85);
        cairo_set_font_size(cr, 36.0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
        cairo_text_extents_t te;
        const char *msg = "🎉  SOLVED!";
        cairo_text_extents(cr, msg, &te);
        cairo_move_to(cr,
                      CANVAS_W / 2.0 - te.width / 2.0 - te.x_bearing,
                      CANVAS_H / 2.0 - te.height / 2.0 - te.y_bearing);
        cairo_show_text(cr, msg);
    }

    return FALSE;
}

/* ============================================================
 *  Button callbacks
 * ============================================================ */
static void on_next(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (current_step < move_count) {
        apply_move(current_step);
        current_step++;
        update_ui();
        gtk_widget_queue_draw(canvas);
    }
}

static void on_prev(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (current_step > 0) {
        current_step--;
        undo_move(current_step);
        update_ui();
        gtk_widget_queue_draw(canvas);
    }
}

static gboolean auto_step(gpointer data)
{
    (void)data;
    on_next(NULL, NULL);
    if (current_step >= move_count || !auto_running) {
        auto_running  = FALSE;
        auto_timer_id = 0;
        gtk_button_set_label(GTK_BUTTON(btn_auto), "▶  Auto Play");
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void on_auto(GtkWidget *w, gpointer data)
{
    (void)data;
    if (auto_running) {
        /* Pause */
        g_source_remove(auto_timer_id);
        auto_timer_id = 0;
        auto_running  = FALSE;
        gtk_button_set_label(GTK_BUTTON(w), "▶  Auto Play");
    } else {
        if (current_step >= move_count) return;   /* already done */
        auto_running = TRUE;
        gtk_button_set_label(GTK_BUTTON(w), "⏸  Pause");
        double speed = gtk_range_get_value(GTK_RANGE(speed_scale)); /* 1–10 */
        /* Map speed 1–10 → delay 1500 ms – 50 ms */
        guint delay = (guint)(1550 - speed * 150);
        auto_timer_id = g_timeout_add(delay, auto_step, NULL);
    }
}

static void on_reset(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    if (auto_running) {
        g_source_remove(auto_timer_id);
        auto_timer_id = 0;
        auto_running  = FALSE;
        gtk_button_set_label(GTK_BUTTON(btn_auto), "▶  Auto Play");
    }
    reset_game();
    update_ui();
    gtk_widget_queue_draw(canvas);
}

/* ──────────────────────────────────────────────
 *  Speed slider changed while auto-playing
 * ────────────────────────────────────────────── */
static void on_speed_changed(GtkRange *range, gpointer data)
{
    (void)data;
    if (!auto_running) return;
    /* Restart timer with new interval */
    g_source_remove(auto_timer_id);
    double speed  = gtk_range_get_value(range);
    guint  delay  = (guint)(1550 - speed * 150);
    auto_timer_id = g_timeout_add(delay, auto_step, NULL);
}

/* ──────────────────────────────────────────────
 *  About dialog
 * ────────────────────────────────────────────── */
static void on_about(GtkWidget *w, gpointer data)
{
    (void)w; (void)data;
    GtkWidget *dlg = gtk_message_dialog_new(
        NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Towers of Hanoi  —  GTK3 Edition\n\n"
        "Classic recursive puzzle visualised with Cairo.\n\n"
        "• Choose number of disks (1–8)\n"
        "• Choose start and destination peg\n"
        "• Step through or auto-play the solution\n\n"
        "Minimum moves = 2ⁿ − 1"
    );
    gtk_window_set_title(GTK_WINDOW(dlg), "About");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

/* ============================================================
 *  CSS styling
 * ============================================================ */
static void apply_css(void)
{
    GtkCssProvider *prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(prov,
        "window {"
        "  background-color: #12121e;"
        "}"
        "button {"
        "  background: linear-gradient(to bottom, #3a3a5e, #1e1e36);"
        "  color: #d0d0ff;"
        "  border: 1px solid #5050a0;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "}"
        "button:hover {"
        "  background: linear-gradient(to bottom, #5050a0, #2a2a60);"
        "  color: #ffffff;"
        "}"
        "button:disabled {"
        "  opacity: 0.35;"
        "}"
        "label {"
        "  color: #c0c0e8;"
        "  font-size: 13px;"
        "}"
        "spinbutton {"
        "  background-color: #1e1e36;"
        "  color: #d0d0ff;"
        "  border: 1px solid #5050a0;"
        "  border-radius: 4px;"
        "}"
        "combobox {"
        "  background-color: #1e1e36;"
        "  color: #d0d0ff;"
        "}"
        "scale trough {"
        "  background-color: #2a2a50;"
        "}"
        "scale highlight {"
        "  background-color: #6060c0;"
        "}"
        , -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(prov);
}

/* ============================================================
 *  main
 * ============================================================ */
int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    apply_css();

    /* ── Main window ── */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Towers of Hanoi");
    gtk_window_set_default_size(GTK_WINDOW(window), CANVAS_W, CANVAS_H + 160);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(window), 12);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* ── Outer vertical box ── */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* ── Canvas ── */
    canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(canvas, CANVAS_W, CANVAS_H);
    g_signal_connect(canvas, "draw", G_CALLBACK(on_draw), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), canvas, TRUE, TRUE, 0);

    /* ── Info bar ── */
    GtkWidget *info_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(vbox), info_box, FALSE, FALSE, 2);

    lbl_step = gtk_label_new("Step 0 / 0");
    gtk_label_set_use_markup(GTK_LABEL(lbl_step), TRUE);
    gtk_box_pack_start(GTK_BOX(info_box), lbl_step, FALSE, FALSE, 0);

    lbl_move = gtk_label_new("Initial state");
    gtk_label_set_use_markup(GTK_LABEL(lbl_move), TRUE);
    gtk_box_pack_start(GTK_BOX(info_box), lbl_move, TRUE, TRUE, 0);

    /* ── Settings row ── */
    GtkWidget *cfg_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(vbox), cfg_box, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(cfg_box),
        gtk_label_new("Disks:"), FALSE, FALSE, 0);
    spin_disks = gtk_spin_button_new_with_range(1, MAX_DISKS, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_disks), 3);
    gtk_box_pack_start(GTK_BOX(cfg_box), spin_disks, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(cfg_box),
        gtk_label_new("  From:"), FALSE, FALSE, 0);
    combo_from = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_from), "Peg 1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_from), "Peg 2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_from), "Peg 3");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_from), 0);
    gtk_box_pack_start(GTK_BOX(cfg_box), combo_from, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(cfg_box),
        gtk_label_new("  To:"), FALSE, FALSE, 0);
    combo_to = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_to), "Peg 1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_to), "Peg 2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_to), "Peg 3");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_to), 2);
    gtk_box_pack_start(GTK_BOX(cfg_box), combo_to, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(cfg_box),
        gtk_label_new("  Speed:"), FALSE, FALSE, 0);
    speed_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
                                           1.0, 10.0, 0.5);
    gtk_range_set_value(GTK_RANGE(speed_scale), 4.0);
    gtk_widget_set_size_request(speed_scale, 110, -1);
    gtk_scale_set_draw_value(GTK_SCALE(speed_scale), FALSE);
    g_signal_connect(speed_scale, "value-changed",
                     G_CALLBACK(on_speed_changed), NULL);
    gtk_box_pack_start(GTK_BOX(cfg_box), speed_scale, FALSE, FALSE, 0);

    /* ── Button row ── */
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), btn_box, FALSE, FALSE, 4);

    btn_reset = gtk_button_new_with_label("↺  Reset");
    g_signal_connect(btn_reset, "clicked", G_CALLBACK(on_reset), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_reset, FALSE, FALSE, 0);

    btn_prev = gtk_button_new_with_label("◀  Prev");
    g_signal_connect(btn_prev, "clicked", G_CALLBACK(on_prev), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_prev, FALSE, FALSE, 0);

    btn_next = gtk_button_new_with_label("Next  ▶");
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_next), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_next, FALSE, FALSE, 0);

    btn_auto = gtk_button_new_with_label("▶  Auto Play");
    g_signal_connect(btn_auto, "clicked", G_CALLBACK(on_auto), NULL);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_auto, FALSE, FALSE, 0);

    GtkWidget *btn_about = gtk_button_new_with_label("ℹ  About");
    g_signal_connect(btn_about, "clicked", G_CALLBACK(on_about), NULL);
    gtk_box_pack_end(GTK_BOX(btn_box), btn_about, FALSE, FALSE, 0);

    /* ── Initialise game state ── */
    reset_game();
    update_ui();

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
