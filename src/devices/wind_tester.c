/** @file
    Cotech 36-7959 Weatherstation.

    Copyright (C) 2020 Andreas Holmberg <andreas@burken.se>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

static int wind_test_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    uint8_t const preamble[] = {0x5f, 0xb4}; // 12 bits

    int r = -1;
    uint8_t b[4];
    data_t *data;

    if (bitbuffer->num_rows > 2) {
        return DECODE_ABORT_EARLY;
    }


    if (bitbuffer->bits_per_row[0] < 47 && bitbuffer->bits_per_row[1] < 47) {
        return DECODE_ABORT_EARLY;
    }

    for (int i = 0; i < bitbuffer->num_rows; ++i) {
        unsigned pos = bitbuffer_search(bitbuffer, i, 0, preamble, 16);
        pos += 16;

        if (pos + 32 > bitbuffer->bits_per_row[i])
            continue; // too short or not found

        r = i;
        bitbuffer_extract_bytes(bitbuffer, i, pos, b, 32);
        decoder_log_bitrow(decoder, 0, __func__, b, sizeof(b) * 8, "MSG");
        break;
    }

    if (r < 0) {
        decoder_log(decoder, 2, __func__, "Couldn't find preamble");
        return DECODE_FAIL_SANITY;
    }

    if (crc8(b, 4, 0x31, 0xc0)) {
        decoder_log(decoder, 2, __func__, "CRC8 fail");
        return DECODE_FAIL_MIC;
    }

    // Extract data from buffer
    int rotations = b[0];
    int wind = b[1];
    int gust = b[2];

    /* clang-format off */
    data = data_make(
            "model",            "",                 DATA_STRING, "Wind test",
            //"subtype",          "Type code",        DATA_INT, subtype,
            "rotations",     "Rotations",             DATA_FORMAT, "%.1f", DATA_DOUBLE, rotations * 0.1f,
            "wind",     "Wind",             DATA_FORMAT, "%.1f km/h", DATA_DOUBLE, wind * 0.1f,
            "gust",        "Gust",  DATA_FORMAT, "%.1f km/h", DATA_DOUBLE, gust * 0.1f,
            "mic",              "Integrity",        DATA_STRING, "CRC",
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const wind_test_output_fields[] = {
        "model",
        //"subtype",
        "rotations",
        "wind",
        "gust",
        "mic",
        NULL,
};

r_device const cotech_36_7959 = {
        .name        = "Wind test",
        .modulation  = OOK_PULSE_MANCHESTER_ZEROBIT,
        .short_width = 500,
        .long_width  = 0,    // not used
        .gap_limit   = 1200, // Not used
        .reset_limit = 1200, // Packet gap is 5400 us.
        .decode_fn   = &wind_test_decode,
        .fields      = wind_test_output_fields,
};
