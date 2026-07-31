void game_draw(Game* g, RenderBuffer* rb) {
    graphics_clear(rb, 0xFFCCCCCC);
    if (g->player.tex_ready)
        graphics_draw_texture_ex(rb, (int)g->player.x, (int)g->player.y,
                                 g->player.texture, g->player.tex_width, g->player.tex_height,
                                 g->player.angle, g->player.scale);
    else
        graphics_draw_rect(rb, (int)g->player.x, (int)g->player.y, 80, 0xFFEE7722);
    ui_draw_joystick(rb, &g->joy);
    char fps[32];
    int fps_int = (int)(g->fps + 0.5f);   // округление
    snprintf(fps, sizeof(fps), "FPS: %d", fps_int);
    font_draw_text(g->font, rb, rb->width-120, 40, fps, 0xFF000000); // чёрный и ниже
}
