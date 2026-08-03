#pragma once
#include "receiver_model.hpp"

namespace portapack {

extern ReceiverModel receiver_model;

namespace spi_flash {
    struct image_tag_t { char tag[4]; };
    constexpr image_tag_t image_tag_nfm_audio{{'P','N','F','M'}};
    constexpr image_tag_t image_tag_wfm_audio{{'P','W','F','M'}};
}

}  // namespace portapack
