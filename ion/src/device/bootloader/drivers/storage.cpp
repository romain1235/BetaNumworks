#include <ion/storage.h>

namespace Ion {

// TODO: Check if the storage is initialized at runtime and not at compile time:
// As far as I can see, it break build for N0110 legacy internal storage and
// N0100 due to lack of storage, so it's probably wasting space.
uint32_t __attribute__((section(".static_storage"))) staticStorageArea[sizeof(Storage)/sizeof(uint32_t)] = {0};

}
