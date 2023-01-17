#ifndef _HDR_H_
#define _HDR_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/* clang-format off
High level syntax

image:   | SOI | Frame | EOI |

frame:   | [Tables/misc.] | Frame header | SCAN0 | [DNL] | SCAN1 | SCAN2 | .. | SCANn |

scan:    | [Tables/misc.] | Scan header | [ECS0 | RST0 | ECS1 | RST1 | .. ECSn-1 | RSTn-1] | ECSn |

ECS:     | <MCU0> <MCU1> <MCU2> ... <MCUn>

clang-format on */

struct JPEG {
};

/*

Header definitions

Frame header:
    | SOFn | Lf | P | Y | X | Nf | Component-spec params |
*/

struct ComponentParam {
    uint8_t idx;
    uint8_t h_smaple_factor : 4;
    uint8_t v_sample_factor : 4;
    uint8_t quantize_table_idx;
};

struct FrameHeader {
    uint16_t start_marker;
    uint16_t header_len;
    uint8_t sample_precision;
    uint16_t line_height;
    uint16_t line_width;
    uint8_t components;
    struct ComponentParam comp_params[3];
};

struct ScanComponentParam {
    uint8_t idx;
    uint8_t dc_tbl_idx : 4;
    uint8_t ac_tbl_idx : 4;
};

struct ScanHeader {
    uint16_t start_marker;
    uint16_t header_len;
    uint8_t components;

    struct ScanComponentParam comp_params[3];

    uint8_t start_spectral;
    uint8_t endof_spectral;
    uint8_t ah : 4;
    uint8_t al : 4;
};

#ifdef __cplusplus
}
#endif
#endif
