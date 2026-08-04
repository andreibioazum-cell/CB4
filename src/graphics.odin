package main

Point :: struct {
    x: int,
    y: int,
}

Render_Buffer :: struct {
    pixels: [^]u32,
    width:  int,
    height: int,
    stride: int,
}

clamp_int :: proc(value, low, high: int) -> int {
    if value < low {
        return low
    }
    if value > high {
        return high
    }
    return value
}

abs_int :: proc(value: int) -> int {
    if value < 0 {
        return -value
    }
    return value
}

set_pixel :: proc(buffer: ^Render_Buffer, x, y: int, color: u32) {
    if x < 0 || y < 0 || x >= buffer.width || y >= buffer.height {
        return
    }
    buffer.pixels[y * buffer.stride + x] = color
}

graphics_clear :: proc(buffer: ^Render_Buffer, color: u32) {
    if buffer.pixels == nil {
        return
    }
    for y := 0; y < buffer.height; y += 1 {
        row := y * buffer.stride
        for x := 0; x < buffer.width; x += 1 {
            buffer.pixels[row + x] = color
        }
    }
}

graphics_fill_rect :: proc(buffer: ^Render_Buffer, x, y, width, height: int, color: u32) {
    x0 := clamp_int(x, 0, buffer.width)
    y0 := clamp_int(y, 0, buffer.height)
    x1 := clamp_int(x + width, 0, buffer.width)
    y1 := clamp_int(y + height, 0, buffer.height)
    if x0 >= x1 || y0 >= y1 {
        return
    }
    for py := y0; py < y1; py += 1 {
        row := py * buffer.stride
        for px := x0; px < x1; px += 1 {
            buffer.pixels[row + px] = color
        }
    }
}

graphics_draw_rect_centered :: proc(buffer: ^Render_Buffer, cx, cy, size: int, color: u32) {
    half := size / 2
    graphics_fill_rect(buffer, cx - half, cy - half, size, size, color)
}

graphics_draw_circle :: proc(buffer: ^Render_Buffer, cx, cy, radius: int, color: u32) {
    if radius <= 0 {
        return
    }
    radius_squared := radius * radius
    for y := -radius; y <= radius; y += 1 {
        for x := -radius; x <= radius; x += 1 {
            if x*x + y*y <= radius_squared {
                set_pixel(buffer, cx + x, cy + y, color)
            }
        }
    }
}

graphics_draw_ring :: proc(buffer: ^Render_Buffer, cx, cy, radius, thickness: int, color: u32) {
    if radius <= 0 {
        return
    }
    outer := radius * radius
    inner_radius := radius - thickness
    if inner_radius < 0 {
        inner_radius = 0
    }
    inner := inner_radius * inner_radius
    for y := -radius; y <= radius; y += 1 {
        for x := -radius; x <= radius; x += 1 {
            distance := x*x + y*y
            if distance <= outer && distance >= inner {
                set_pixel(buffer, cx + x, cy + y, color)
            }
        }
    }
}

graphics_draw_line :: proc(buffer: ^Render_Buffer, from, to: Point, color: u32) {
    x0, y0 := from.x, from.y
    x1, y1 := to.x, to.y
    dx := abs_int(x1 - x0)
    sx := 1
    if x0 > x1 {
        sx = -1
    }
    dy := -abs_int(y1 - y0)
    sy := 1
    if y0 > y1 {
        sy = -1
    }
    error := dx + dy

    for {
        set_pixel(buffer, x0, y0, color)
        if x0 == x1 && y0 == y1 {
            break
        }
        twice_error := 2 * error
        if twice_error >= dy {
            error += dy
            x0 += sx
        }
        if twice_error <= dx {
            error += dx
            y0 += sy
        }
    }
}

edge_function :: proc(a, b, point: Point) -> int {
    return (point.x - a.x) * (b.y - a.y) - (point.y - a.y) * (b.x - a.x)
}

graphics_draw_triangle :: proc(buffer: ^Render_Buffer, a, b, c: Point, color: u32) {
    min_x := a.x
    if b.x < min_x { min_x = b.x }
    if c.x < min_x { min_x = c.x }
    max_x := a.x
    if b.x > max_x { max_x = b.x }
    if c.x > max_x { max_x = c.x }
    min_y := a.y
    if b.y < min_y { min_y = b.y }
    if c.y < min_y { min_y = c.y }
    max_y := a.y
    if b.y > max_y { max_y = b.y }
    if c.y > max_y { max_y = c.y }

    min_x = clamp_int(min_x, 0, buffer.width - 1)
    max_x = clamp_int(max_x, 0, buffer.width - 1)
    min_y = clamp_int(min_y, 0, buffer.height - 1)
    max_y = clamp_int(max_y, 0, buffer.height - 1)

    area := edge_function(a, b, c)
    if area == 0 {
        return
    }
    for y := min_y; y <= max_y; y += 1 {
        for x := min_x; x <= max_x; x += 1 {
            point := Point{x, y}
            w0 := edge_function(b, c, point)
            w1 := edge_function(c, a, point)
            w2 := edge_function(a, b, point)
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0) {
                set_pixel(buffer, x, y, color)
            }
        }
    }
}
