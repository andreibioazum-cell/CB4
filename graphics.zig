const std = @import("std");

/// RenderBuffer representation matching C RenderBuffer struct
pub const RenderBuffer = extern struct {
    pixels: [*]u32,
    width: c_int,
    height: c_int,
    stride: c_int,
};

/// Clears the entire render buffer with a 32-bit ARGB/RGBA color
pub export fn graphics_clear(rb: ?*RenderBuffer, color: u32) void {
    const b = rb orelse return;
    if (b.width <= 0 or b.height <= 0 or b.stride <= 0) return;
    const total: usize = @as(usize, @intCast(b.stride)) * @as(usize, @intCast(b.height));
    @memset(b.pixels[0..total], color);
}

/// Draws a solid rectangle centered at (x, y) with side length `size`
pub export fn graphics_draw_rect(rb: ?*RenderBuffer, x: c_int, y: c_int, size: c_int, color: u32) void {
    const b = rb orelse return;
    if (size <= 0 or b.width <= 0 or b.height <= 0) return;
    const half = @divTrunc(size, 2);
    var x1: c_int = x - half;
    var x2: c_int = x + half;
    var y1: c_int = y - half;
    var y2: c_int = y + half;

    if (x1 < 0) x1 = 0;
    if (x2 > b.width) x2 = b.width;
    if (y1 < 0) y1 = 0;
    if (y2 > b.height) y2 = b.height;

    if (x1 >= x2 or y1 >= y2) return;

    const u_stride: usize = @intCast(b.stride);
    const u_x1: usize = @intCast(x1);
    const u_x2: usize = @intCast(x2);
    const span_len: usize = u_x2 - u_x1;

    var row = y1;
    while (row < y2) : (row += 1) {
        const row_offset = @as(usize, @intCast(row)) * u_stride + u_x1;
        @memset(b.pixels[row_offset .. row_offset + span_len], color);
    }
}

/// Draws a solid rectangle with top-left corner at (x, y) and dimensions (w, h)
pub export fn graphics_draw_rect_exact(rb: ?*RenderBuffer, x: c_int, y: c_int, w: c_int, h: c_int, color: u32) void {
    const b = rb orelse return;
    if (w <= 0 or h <= 0 or b.width <= 0 or b.height <= 0) return;
    var x1: c_int = x;
    var x2: c_int = x + w;
    var y1: c_int = y;
    var y2: c_int = y + h;

    if (x1 < 0) x1 = 0;
    if (x2 > b.width) x2 = b.width;
    if (y1 < 0) y1 = 0;
    if (y2 > b.height) y2 = b.height;

    if (x1 >= x2 or y1 >= y2) return;

    const u_stride: usize = @intCast(b.stride);
    const u_x1: usize = @intCast(x1);
    const u_x2: usize = @intCast(x2);
    const span_len: usize = u_x2 - u_x1;

    var row = y1;
    while (row < y2) : (row += 1) {
        const row_offset = @as(usize, @intCast(row)) * u_stride + u_x1;
        @memset(b.pixels[row_offset .. row_offset + span_len], color);
    }
}

/// Draws a rectangle border outline with specified line thickness
pub export fn graphics_draw_rect_lines(rb: ?*RenderBuffer, x: c_int, y: c_int, w: c_int, h: c_int, thickness: c_int, color: u32) void {
    if (thickness <= 0 or w <= 0 or h <= 0) return;
    if (thickness * 2 >= w or thickness * 2 >= h) {
        graphics_draw_rect_exact(rb, x, y, w, h, color);
        return;
    }
    // Top border
    graphics_draw_rect_exact(rb, x, y, w, thickness, color);
    // Bottom border
    graphics_draw_rect_exact(rb, x, y + h - thickness, w, thickness, color);
    // Left border
    graphics_draw_rect_exact(rb, x, y + thickness, thickness, h - 2 * thickness, color);
    // Right border
    graphics_draw_rect_exact(rb, x + w - thickness, y + thickness, thickness, h - 2 * thickness, color);
}

/// Draws a filled circle centered at (cx, cy) with radius `r` using span-filling
pub export fn graphics_draw_circle(rb: ?*RenderBuffer, cx: c_int, cy: c_int, r: c_int, color: u32) void {
    const b = rb orelse return;
    if (r <= 0 or b.width <= 0 or b.height <= 0) return;
    const r2: i64 = @as(i64, r) * @as(i64, r);
    const u_stride: usize = @intCast(b.stride);

    var y: c_int = -r;
    while (y <= r) : (y += 1) {
        const sy = cy + y;
        if (sy < 0 or sy >= b.height) continue;
        const y2: i64 = @as(i64, y) * @as(i64, y);
        const rem = r2 - y2;
        if (rem < 0) continue;

        const f_rem: f32 = @floatFromInt(rem);
        const dx: c_int = @intFromFloat(@sqrt(f_rem));

        var x1 = cx - dx;
        var x2 = cx + dx + 1;
        if (x1 < 0) x1 = 0;
        if (x2 > b.width) x2 = b.width;
        if (x1 >= x2) continue;

        const row_offset = @as(usize, @intCast(sy)) * u_stride + @as(usize, @intCast(x1));
        const span_len: usize = @intCast(x2 - x1);
        @memset(b.pixels[row_offset .. row_offset + span_len], color);
    }
}

/// Draws a ring (circle outline) centered at (cx, cy) with radius `r` and thickness `t`
pub export fn graphics_draw_ring(rb: ?*RenderBuffer, cx: c_int, cy: c_int, r: c_int, thickness: c_int, color: u32) void {
    const b = rb orelse return;
    if (r <= 0 or thickness <= 0 or b.width <= 0 or b.height <= 0) return;
    const ro: i64 = @as(i64, r) * @as(i64, r);
    const inner_r: c_int = if (r > thickness) r - thickness else 0;
    const ri: i64 = @as(i64, inner_r) * @as(i64, inner_r);
    const u_stride: usize = @intCast(b.stride);

    var y: c_int = -r;
    while (y <= r) : (y += 1) {
        const sy = cy + y;
        if (sy < 0 or sy >= b.height) continue;
        const row_offset = @as(usize, @intCast(sy)) * u_stride;
        const y2: i64 = @as(i64, y) * @as(i64, y);

        var x: c_int = -r;
        while (x <= r) : (x += 1) {
            const sx = cx + x;
            if (sx < 0 or sx >= b.width) continue;
            const d = @as(i64, x) * @as(i64, x) + y2;
            if (d <= ro and d >= ri) {
                b.pixels[row_offset + @as(usize, @intCast(sx))] = color;
            }
        }
    }
}

/// Draws a 1px line using Bresenham's line algorithm
pub export fn graphics_draw_line(rb: ?*RenderBuffer, x0_in: c_int, y0_in: c_int, x1_in: c_int, y1_in: c_int, color: u32) void {
    const b = rb orelse return;
    if (b.width <= 0 or b.height <= 0) return;

    var x0 = x0_in;
    var y0 = y0_in;
    const x1 = x1_in;
    const y1 = y1_in;

    const dx: c_int = if (x1 >= x0) x1 - x0 else x0 - x1;
    const sx: c_int = if (x0 < x1) 1 else -1;
    const dy: c_int = if (y1 >= y0) -(y1 - y0) else -(y0 - y1);
    const sy: c_int = if (y0 < y1) 1 else -1;
    var err: c_int = dx + dy;

    const u_stride: usize = @intCast(b.stride);

    while (true) {
        if (x0 >= 0 and x0 < b.width and y0 >= 0 and y0 < b.height) {
            const offset = @as(usize, @intCast(y0)) * u_stride + @as(usize, @intCast(x0));
            b.pixels[offset] = color;
        }
        if (x0 == x1 and y0 == y1) break;
        const e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/// Draws a direct texture blit at (x, y) with alpha blending
pub export fn graphics_draw_texture(
    rb: ?*RenderBuffer,
    x: c_int,
    y: c_int,
    tex: ?[*]const u32,
    tw: c_int,
    th: c_int,
) void {
    const b = rb orelse return;
    const t = tex orelse return;
    if (tw <= 0 or th <= 0 or b.width <= 0 or b.height <= 0) return;

    var x1 = x;
    var x2 = x + tw;
    var y1 = y;
    var y2 = y + th;

    if (x1 < 0) x1 = 0;
    if (x2 > b.width) x2 = b.width;
    if (y1 < 0) y1 = 0;
    if (y2 > b.height) y2 = b.height;

    if (x1 >= x2 or y1 >= y2) return;

    const u_stride: usize = @intCast(b.stride);
    const u_tw: usize = @intCast(tw);

    var row = y1;
    while (row < y2) : (row += 1) {
        const tex_row = @as(usize, @intCast(row - y)) * u_tw;
        const dst_row = @as(usize, @intCast(row)) * u_stride;

        var col = x1;
        while (col < x2) : (col += 1) {
            const tex_col = @as(usize, @intCast(col - x));
            const p = t[tex_row + tex_col];
            const alpha: u32 = (p >> 24) & 0xFF;
            if (alpha == 255) {
                b.pixels[dst_row + @as(usize, @intCast(col))] = p;
            } else if (alpha > 0) {
                const dst_idx = dst_row + @as(usize, @intCast(col));
                const dst = b.pixels[dst_idx];
                const inv_a: u32 = 255 - alpha;

                const sr = (p >> 16) & 0xFF;
                const sg = (p >> 8) & 0xFF;
                const sb = p & 0xFF;

                const dr = (dst >> 16) & 0xFF;
                const dg = (dst >> 8) & 0xFF;
                const db = dst & 0xFF;

                const out_r = (sr * alpha + dr * inv_a) / 255;
                const out_g = (sg * alpha + dg * inv_a) / 255;
                const out_b = (sb * alpha + db * inv_a) / 255;

                b.pixels[dst_idx] = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
            }
        }
    }
}

/// Draws a texture centered at (cx, cy) with rotation (angle in radians), scaling, and alpha blending
pub export fn graphics_draw_texture_ex(
    rb: ?*RenderBuffer,
    cx: c_int,
    cy: c_int,
    tex: ?[*]const u32,
    tw: c_int,
    th: c_int,
    angle: f32,
    scale: f32,
) void {
    const b = rb orelse return;
    const t = tex orelse return;
    if (tw <= 0 or th <= 0 or scale <= 0.0 or b.width <= 0 or b.height <= 0) return;

    const ftw: f32 = @floatFromInt(tw);
    const fth: f32 = @floatFromInt(th);
    const sw: c_int = @intFromFloat(ftw * scale);
    const sh: c_int = @intFromFloat(fth * scale);
    if (sw <= 0 or sh <= 0) return;

    const ca = @cos(angle);
    const sa = @sin(angle);

    var l = cx - @divTrunc(sw, 2);
    var tp = cy - @divTrunc(sh, 2);
    var r = cx + @divTrunc(sw, 2);
    var bt = cy + @divTrunc(sh, 2);

    if (l < 0) l = 0;
    if (tp < 0) tp = 0;
    if (r > b.width) r = b.width;
    if (bt > b.height) bt = b.height;
    if (l >= r or tp >= bt) return;

    const htw: f32 = ftw * 0.5;
    const hth: f32 = fth * 0.5;
    const inv: f32 = 1.0 / scale;
    const fcx: f32 = @floatFromInt(cx);
    const fcy: f32 = @floatFromInt(cy);
    const u_stride: usize = @intCast(b.stride);
    const u_tw: usize = @intCast(tw);

    var y = tp;
    while (y < bt) : (y += 1) {
        const fy: f32 = @floatFromInt(y);
        const dy = fy - fcy;
        const row_offset = @as(usize, @intCast(y)) * u_stride;

        var x = l;
        while (x < r) : (x += 1) {
            const fx: f32 = @floatFromInt(x);
            const dx = fx - fcx;

            const sx = dx * ca + dy * sa;
            const sy = -dx * sa + dy * ca;

            const tx = sx * inv + htw;
            const ty = sy * inv + hth;

            const ix: c_int = @intFromFloat(tx + 0.5);
            const iy: c_int = @intFromFloat(ty + 0.5);

            if (ix >= 0 and ix < tw and iy >= 0 and iy < th) {
                const tex_idx = @as(usize, @intCast(iy)) * u_tw + @as(usize, @intCast(ix));
                const p = t[tex_idx];
                const alpha: u32 = (p >> 24) & 0xFF;
                if (alpha == 255) {
                    b.pixels[row_offset + @as(usize, @intCast(x))] = p;
                } else if (alpha > 0) {
                    const dst_idx = row_offset + @as(usize, @intCast(x));
                    const dst = b.pixels[dst_idx];
                    const inv_a: u32 = 255 - alpha;

                    const sr = (p >> 16) & 0xFF;
                    const sg = (p >> 8) & 0xFF;
                    const sb = p & 0xFF;

                    const dr = (dst >> 16) & 0xFF;
                    const dg = (dst >> 8) & 0xFF;
                    const db = dst & 0xFF;

                    const out_r = (sr * alpha + dr * inv_a) / 255;
                    const out_g = (sg * alpha + dg * inv_a) / 255;
                    const out_b = (sb * alpha + db * inv_a) / 255;

                    b.pixels[dst_idx] = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
                }
            }
        }
    }
}
