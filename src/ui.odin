package main

import "core:math"

Joystick :: struct {
    center_x:   int,
    center_y:   int,
    radius:     int,
    dir_x:      f32,
    dir_y:      f32,
    touch_off_x: f32,
    touch_off_y: f32,
    active:     bool,
}

Button :: struct {
    x:       int,
    y:       int,
    radius:  int,
    pressed: bool,
}

ui_distance_squared :: proc(x0, y0, x1, y1: f32) -> f32 {
    dx := x1 - x0
    dy := y1 - y0
    return dx*dx + dy*dy
}

ui_joystick_begin :: proc(joystick: ^Joystick, x, y: f32) -> bool {
    reach := f32(joystick.radius) * 1.45
    if ui_distance_squared(x, y, f32(joystick.center_x), f32(joystick.center_y)) > reach*reach {
        return false
    }
    joystick.active = true
    ui_joystick_move(joystick, x, y)
    return true
}

ui_joystick_move :: proc(joystick: ^Joystick, x, y: f32) {
    if !joystick.active {
        return
    }
    dx := x - f32(joystick.center_x)
    dy := y - f32(joystick.center_y)
    distance := math.sqrt_f32(dx*dx + dy*dy)
    if distance < 0.001 {
        joystick.dir_x = 0
        joystick.dir_y = 0
        joystick.touch_off_x = 0
        joystick.touch_off_y = 0
        return
    }
    inverse_distance := 1.0 / distance
    joystick.dir_x = dx * inverse_distance
    joystick.dir_y = dy * inverse_distance
    clamped := distance
    if clamped > f32(joystick.radius) {
        clamped = f32(joystick.radius)
    }
    joystick.touch_off_x = joystick.dir_x * clamped
    joystick.touch_off_y = joystick.dir_y * clamped
}

ui_joystick_end :: proc(joystick: ^Joystick) {
    joystick.active = false
    joystick.dir_x = 0
    joystick.dir_y = 0
    joystick.touch_off_x = 0
    joystick.touch_off_y = 0
}

ui_button_contains :: proc(button: ^Button, x, y: f32) -> bool {
    radius := f32(button.radius)
    return ui_distance_squared(x, y, f32(button.x), f32(button.y)) <= radius*radius
}

ui_button_press :: proc(button: ^Button, x, y: f32) -> bool {
    if !ui_button_contains(button, x, y) {
        return false
    }
    button.pressed = true
    return true
}

ui_button_release :: proc(button: ^Button, x, y: f32) -> bool {
    was_pressed := button.pressed
    button.pressed = false
    return was_pressed && ui_button_contains(button, x, y)
}

ui_draw_joystick :: proc(buffer: ^Render_Buffer, joystick: ^Joystick) {
    graphics_draw_circle(buffer, joystick.center_x, joystick.center_y, joystick.radius + 8, 0xFF0B1220)
    graphics_draw_ring(buffer, joystick.center_x, joystick.center_y, joystick.radius, 4, 0xFF69A7FF)
    knob_x := joystick.center_x + int(joystick.touch_off_x)
    knob_y := joystick.center_y + int(joystick.touch_off_y)
    knob_color := u32(0xFF3B82F6)
    if !joystick.active {
        knob_color = 0xFF2563A8
    }
    graphics_draw_circle(buffer, knob_x, knob_y, joystick.radius / 2, knob_color)
    graphics_draw_ring(buffer, knob_x, knob_y, joystick.radius / 2, 3, 0xFFB9D8FF)
}

ui_draw_button :: proc(buffer: ^Render_Buffer, button: ^Button) {
    color := u32(0xFFE04455)
    if button.pressed {
        color = 0xFFFF8A4C
    }
    graphics_draw_circle(buffer, button.x, button.y, button.radius + 7, 0xFF210F1B)
    graphics_draw_circle(buffer, button.x, button.y, button.radius, color)
    graphics_draw_ring(buffer, button.x, button.y, button.radius, 4, 0xFFFFC4C4)

    scale := button.radius / 30
    if scale < 1 { scale = 1 }
    label := `ATTACK`
    label_width := ui_text_width(label, scale)
    ui_draw_text_outlined(buffer, button.x - label_width/2, button.y - 3*scale, label, scale, 0xFFFFFFFF, 0xFF38111A)
}

ui_glyph :: proc(character: u8) -> [7]u8 {
    switch character {
    case 'A': return [7]u8{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}
    case 'C': return [7]u8{0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F}
    case 'E': return [7]u8{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}
    case 'F': return [7]u8{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}
    case 'K': return [7]u8{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}
    case 'P': return [7]u8{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}
    case 'S': return [7]u8{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}
    case 'T': return [7]u8{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}
    case '0': return [7]u8{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}
    case '1': return [7]u8{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}
    case '2': return [7]u8{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}
    case '3': return [7]u8{0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}
    case '4': return [7]u8{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}
    case '5': return [7]u8{0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}
    case '6': return [7]u8{0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}
    case '7': return [7]u8{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}
    case '8': return [7]u8{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}
    case '9': return [7]u8{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}
    case ':': return [7]u8{0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}
    case ' ': return [7]u8{0, 0, 0, 0, 0, 0, 0}
    }
    return [7]u8{0, 0, 0, 0, 0, 0, 0}
}

ui_text_width :: proc(text: string, scale: int) -> int {
    if len(text) == 0 {
        return 0
    }
    return (len(text) * 6 - 1) * scale
}

ui_draw_glyph :: proc(buffer: ^Render_Buffer, x, y: int, character: u8, scale: int, color: u32) {
    if scale <= 0 {
        return
    }
    glyph := ui_glyph(character)
    for row := 0; row < 7; row += 1 {
        bits := glyph[row]
        for column := 0; column < 5; column += 1 {
            mask := u8(1 << uint(4 - column))
            if bits & mask != 0 {
                graphics_fill_rect(buffer, x + column*scale, y + row*scale, scale, scale, color)
            }
        }
    }
}

ui_draw_text :: proc(buffer: ^Render_Buffer, x, y: int, text: string, scale: int, color: u32) {
    if scale <= 0 {
        return
    }
    cursor_x := x
    for index := 0; index < len(text); index += 1 {
        ui_draw_glyph(buffer, cursor_x, y, text[index], scale, color)
        cursor_x += 6 * scale
    }
}

ui_draw_text_outlined :: proc(buffer: ^Render_Buffer, x, y: int, text: string, scale: int, color, outline: u32) {
    for dy := -1; dy <= 1; dy += 1 {
        for dx := -1; dx <= 1; dx += 1 {
            if dx != 0 || dy != 0 {
                ui_draw_text(buffer, x + dx*scale, y + dy*scale, text, scale, outline)
            }
        }
    }
    ui_draw_text(buffer, x, y, text, scale, color)
}

ui_draw_number :: proc(buffer: ^Render_Buffer, x, y, value_in, scale: int, color, outline: u32) {
    value := value_in
    if value < 0 {
        value = 0
    }
    digits: [12]u8
    count := 0
    if value == 0 {
        digits[0] = '0'
        count = 1
    } else {
        for value > 0 && count < len(digits) {
            digits[count] = u8(value % 10) + '0'
            value /= 10
            count += 1
        }
    }
    for i := 0; i < count; i += 1 {
        character := digits[count - i - 1]
        glyph_x := x + i*6*scale
        for dy := -1; dy <= 1; dy += 1 {
            for dx := -1; dx <= 1; dx += 1 {
                if dx != 0 || dy != 0 {
                    ui_draw_glyph(buffer, glyph_x + dx*scale, y + dy*scale, character, scale, outline)
                }
            }
        }
        ui_draw_glyph(buffer, glyph_x, y, character, scale, color)
    }
}
