#!/usr/bin/env python3

import struct
import sys
import zlib


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def paeth(left, above, upper_left):
    estimate = left + above - upper_left
    distances = (
        abs(estimate - left),
        abs(estimate - above),
        abs(estimate - upper_left),
    )
    return (left, above, upper_left)[distances.index(min(distances))]


def convert(source_path, output_path, rgba_key_white=False):
    data = open(source_path, "rb").read()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("input is not a PNG image")

    offset = len(PNG_SIGNATURE)
    compressed = bytearray()
    width = height = bit_depth = color_type = interlace = None
    while offset + 12 <= len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        chunk_type = data[offset + 4 : offset + 8]
        chunk_start = offset + 8
        chunk_end = chunk_start + length
        if chunk_end + 4 > len(data):
            raise ValueError("truncated PNG chunk")
        chunk = data[chunk_start:chunk_end]

        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, compression, filtering, interlace = (
                struct.unpack(">IIBBBBB", chunk)
            )
            if compression != 0 or filtering != 0:
                raise ValueError("unsupported PNG compression or filter method")
        elif chunk_type == b"IDAT":
            compressed.extend(chunk)
        elif chunk_type == b"IEND":
            break
        offset = chunk_end + 4

    if not width or not height:
        raise ValueError("PNG has no valid image dimensions")
    if bit_depth != 8 or color_type not in (2, 6) or interlace != 0:
        raise ValueError("expected a non-interlaced 8-bit RGB or RGBA PNG")

    channels = 3 if color_type == 2 else 4
    stride = width * channels
    decoded = zlib.decompress(compressed)
    if len(decoded) != height * (stride + 1):
        raise ValueError("PNG pixel data has an unexpected size")

    output_channels = 4 if rgba_key_white else 3
    pixels = bytearray(width * height * output_channels)
    previous = bytearray(stride)
    source_offset = 0
    output_offset = 0
    for _ in range(height):
        filter_type = decoded[source_offset]
        source_offset += 1
        row = bytearray(decoded[source_offset : source_offset + stride])
        source_offset += stride

        for index in range(stride):
            left = row[index - channels] if index >= channels else 0
            above = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = above
            elif filter_type == 3:
                predictor = (left + above) // 2
            elif filter_type == 4:
                predictor = paeth(left, above, upper_left)
            elif filter_type == 0:
                predictor = 0
            else:
                raise ValueError("unsupported PNG row filter")
            row[index] = (row[index] + predictor) & 0xFF

        for x in range(width):
            pixel_offset = x * channels
            pixels[output_offset : output_offset + 3] = row[pixel_offset : pixel_offset + 3]
            if rgba_key_white:
                pixels[output_offset + 3] = 255
                output_offset += 4
            else:
                output_offset += 3
        previous = row

    if rgba_key_white:
        pending = []
        for x in range(width):
            pending.extend((x, (height - 1) * width + x))
        for y in range(height):
            pending.extend((y * width, y * width + width - 1))
        visited = bytearray(width * height)
        while pending:
            pixel_index = pending.pop()
            if visited[pixel_index]:
                continue
            visited[pixel_index] = 1
            offset = pixel_index * 4
            red, green, blue = pixels[offset : offset + 3]
            if red < 240 or green < 240 or blue < 240:
                continue
            pixels[offset + 3] = 0
            x = pixel_index % width
            y = pixel_index // width
            if x:
                pending.append(pixel_index - 1)
            if x + 1 < width:
                pending.append(pixel_index + 1)
            if y:
                pending.append(pixel_index - width)
            if y + 1 < height:
                pending.append(pixel_index + width)

    with open(output_path, "wb") as output:
        output.write(struct.pack("<II", width, height))
        output.write(pixels)


if __name__ == "__main__":
    if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] != "--rgba-key-white"):
        raise SystemExit(
            "usage: png_to_rgb.py INPUT.png OUTPUT.rgb [--rgba-key-white]"
        )
    try:
        convert(sys.argv[1], sys.argv[2], len(sys.argv) == 4)
    except (OSError, ValueError, zlib.error) as error:
        raise SystemExit("wallpaper conversion failed: {}".format(error))
