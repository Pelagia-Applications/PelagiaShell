use std::time::{SystemTime, UNIX_EPOCH};

pub fn shellcore() {
    println!("shellcore initialized");
}

pub fn coinflip() -> &'static str {
    let nanos = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos();

    if nanos % 2 == 0 { "Heads" } else { "Tails" }
}

pub fn roll_dice(sides: u32) -> u32 {
    if sides == 0 {
        return 0;
    }

    let nanos = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos();
    ((nanos % sides as u128) as u32) + 1
}

pub fn uud_encode(input: &str) -> String {
    let mut output = String::new();
    let bytes: Vec<u8> = input.bytes().collect();

    for chunk in bytes.chunks(3) {
        let mut values = [0u8; 4];
        let length = chunk.len();

        values[0] = chunk[0] >> 2;
        values[1] = ((chunk[0] & 0x03) << 4) | if length > 1 { chunk[1] >> 4 } else { 0 };
        values[2] = ((chunk[1] & 0x0F) << 2) | if length > 2 { chunk[2] >> 6 } else { 0 };
        values[3] = chunk.get(2).copied().unwrap_or(0) & 0x3F;

        output.push((values[0] + 0x20) as char);
        output.push((values[1] + 0x20) as char);
        output.push((values[2] + 0x20) as char);
        output.push((values[3] + 0x20) as char);
        output.push('\n');
    }

    output.push('`');
    output.push('\n');
    output
}

pub fn uud_decode(input: &str) -> String {
    let mut output = String::new();
    let mut chars = input.chars().filter(|ch| *ch != '\r' && *ch != '\n').collect::<Vec<_>>();

    if chars.is_empty() {
        return output;
    }

    while chars.len() >= 5 {
        let first = chars[0] as u8;
        let length = if first == b'`' { 0 } else { first - 0x20 };
        if length == 0 {
            break;
        }

        let a = (chars[1] as u8) - 0x20;
        let b = (chars[2] as u8) - 0x20;
        let c = (chars[3] as u8) - 0x20;
        let d = (chars[4] as u8) - 0x20;

        output.push((a << 2 | (b >> 4)) as char);
        if length >= 2 {
            output.push((((b & 0x0F) << 4) | (c >> 2)) as char);
        }
        if length >= 3 {
            output.push((((c & 0x03) << 6) | d) as char);
        }

        chars.drain(0..5);
    }

    output
}
