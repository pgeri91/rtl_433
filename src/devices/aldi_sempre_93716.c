/** @file
    Aldi Sempre Wetterstation 93716.

    Copyright 2024 Gergő P.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/
/**
Aldi Sempre Wetterstation 93716

40-bit one-row data packet format (inclusive ranges, 0-indexed):

|  0-7  | 8 bit  preamble
|  8-15 | 8 bit  humidity
| 16-17 | 2 bit  battery
| 18-19 | 2 bit  channel
| 20-32 | 14 bit temperature in tenths of degrees °C
| 32-40 | 8 bit  checksum, index into checksum table
*/

#include "decoder.h"

static uint8_t const aldi_checksum_table[256] = {97, 80, 3, 50, 165, 148, 199, 246, 216, 233, 186, 139, 28, 45, 126, 79, 34, 19,
64, 113, 230, 215, 132, 181, 155, 170, 249, 200, 95, 110, 61, 12, 231, 214, 133,
180, 35, 18, 65, 112, 94, 111, 60, 13, 154, 171, 248, 201, 164, 149, 198, 247,
96, 81, 2, 51, 29, 44, 127, 78, 217, 232, 187, 138, 92, 109, 62, 15, 152, 169,
250, 203, 229, 212, 135, 182, 33, 16, 67, 114, 31, 46, 125, 76, 219, 234, 185,
136, 166, 151, 196, 245, 98, 83, 0, 49, 218, 235, 184, 137, 30, 47, 124, 77, 99,
82, 1, 48, 167, 150, 197, 244, 153, 168, 251, 202, 93, 108, 63, 14, 32, 17, 66,
115, 228, 213, 134, 183, 27, 42, 121, 72, 223, 238, 189, 140, 162, 147, 192,
241, 102, 87, 4, 53, 88, 105, 58, 11, 156, 173, 254, 207, 225, 208, 131, 178,
37, 20, 71, 118, 157, 172, 255, 206, 89, 104, 59, 10, 36, 21, 70, 119, 224, 209,
130, 179, 222, 239, 188, 141, 26, 43, 120, 73, 103, 86, 5, 52, 163, 146, 193,
240, 38, 23, 68, 117, 226, 211, 128, 177, 159, 174, 253, 204, 91, 106, 57, 8,
101, 84, 7, 54, 161, 144, 195, 242, 220, 237, 190, 143, 24, 41, 122, 75, 160,
145, 194, 243, 100, 85, 6, 55, 25, 40, 123, 74, 221, 236, 191, 142, 227, 210,
129, 176, 39, 22, 69, 116, 90, 107, 56, 9, 158, 175, 252, 205
};

static int aldi_sempre_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    uint8_t const preamble_pattern[] = { 0xf2 };

    if (bitbuffer->num_rows != 1) {
        return DECODE_ABORT_LENGTH;
    }

    unsigned pos = bitbuffer_search(bitbuffer, 0, 0, preamble_pattern, 8);

    // Preamble found ?
    if (pos >= bitbuffer->bits_per_row[0]) {
        decoder_logf(decoder, 2, __func__, "Preamble not found");
        return DECODE_ABORT_EARLY;
    }

    // 40 bits required
    if ((bitbuffer->bits_per_row[0] - pos) < 40) {
        decoder_logf(decoder, 2, __func__, "Too short");
        return DECODE_ABORT_EARLY;
    }

    uint8_t b[5];
    bitbuffer_extract_bytes(bitbuffer, 0, 0 , b, sizeof(b) * 8);
    // invert the bits
    for (int i = 0; i < 5; i++) {
        b[i] = b[i] ^ 0xFF;
    }
    decoder_log_bitrow(decoder, 2, __func__, b, sizeof(b) * 8, "MSG");

    uint8_t checksum_index = b[1] ^ b[2] ^ b[3];
    if (b[4] != aldi_checksum_table[checksum_index]) {
        return DECODE_FAIL_MIC;
    }

    int humidity = b[1];
    int battery = b[2] >> 6;
    int channel = (b[2] >> 4) & 0x3;
    float temperature = 0.0;

    int temp = b[2] & 0xF;
    temp = (temp << 8) | b[3];
    if ((b[2] & 0x8) == 0x8) {
        temperature = (float)(temp ^ 0xFFF) / -10.0;
    } else {
        temperature = (float)temp / 10.0;
    }

    /* clang-format off */
    data_t *data = data_make(
            "model",         "",            DATA_STRING, "Aldi Sempre Wetterstation 93716",
            "battery",       "Battery",     DATA_INT,    battery,
            "channel",       "Channel",     DATA_INT,    channel,
            "humidity",       "Humidity",     DATA_INT,    humidity,
            "temperature_C", "Temperature", DATA_FORMAT, "%.1f C", DATA_DOUBLE, temperature,
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "battery",
        "channel",
        "humidity"
        "temperature_C",
        NULL,
};

r_device const aldi_sempre = {
        .name        = "Aldi Sempre Wetterstation 93716",
        .modulation  = OOK_PULSE_PWM,
        .short_width = 244,
        .long_width  = 608,
        .gap_limit   = 0,
        .reset_limit = 868,
        .sync_width  = 852,
        .decode_fn   = &aldi_sempre_decode,
        .fields      = output_fields,
};
